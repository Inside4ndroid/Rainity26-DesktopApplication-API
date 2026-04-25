#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commoncontrols.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <powerbase.h>
#include <batclass.h>
#include <setupapi.h>
#include <devguid.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <winioctl.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <map>
#include <memory>
#include <algorithm>

#include "GPUMonitor.h"

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

// ── Module handle (captured in DllMain) ────────────────────────────────────
static HMODULE s_hModule = nullptr;

// ── CPU (PDH) ──────────────────────────────────────────────────────────────
static PDH_HQUERY   s_cpuQuery   = nullptr;
static PDH_HCOUNTER s_cpuCounter = nullptr;

// ── Additional PDH Counters ────────────────────────────────────────────────
static PDH_HCOUNTER s_diskReadCounter  = nullptr;
static PDH_HCOUNTER s_diskWriteCounter = nullptr;
static PDH_HCOUNTER s_netSentCounter   = nullptr;
static PDH_HCOUNTER s_netRecvCounter   = nullptr;
static PDH_HCOUNTER s_gpuCounter       = nullptr;
static PDH_HCOUNTER s_gpuMemCounter    = nullptr;

// ── Per-core CPU counters ──────────────────────────────────────────────────
static std::vector<PDH_HCOUNTER> s_cpuCoreCounters;
static int s_coreCount = 0;

// ── Mouse hook ─────────────────────────────────────────────────────────────
static HHOOK             s_mouseHook     = nullptr;
static std::atomic<bool> s_mouseDown     { false };
static std::atomic<bool> s_mouseUp       { false };
static std::atomic<bool> s_hookRunning   { false };
static DWORD             s_hookThreadId  = 0;

static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        if      (wParam == WM_LBUTTONDOWN) s_mouseDown = true;
        else if (wParam == WM_LBUTTONUP)   s_mouseUp   = true;
    }
    return CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
}

static void HookThreadProc(HMODULE hMod)
{
    s_hookThreadId = GetCurrentThreadId();
    s_mouseHook    = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hMod, 0);
    s_hookRunning  = (s_mouseHook != nullptr);

    MSG msg;
    while (s_hookRunning && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (s_mouseHook) {
        UnhookWindowsHookEx(s_mouseHook);
        s_mouseHook = nullptr;
    }
    s_hookRunning = false;
}

// ── Static pixel buffer for GetJumboIcon ──────────────────────────────────
static std::vector<BYTE> s_iconBuf;


// ═══════════════════════════════════════════════════════════════════════════
// Exported API
// ═══════════════════════════════════════════════════════════════════════════
extern "C" {

// ---------------------------------------------------------------------------
// Initialize – opens the PDH query; must be called once at startup.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool Initialize()
{
    if (PdhOpenQuery(nullptr, 0, &s_cpuQuery) != ERROR_SUCCESS)
        return false;

    // CPU Overall
    if (PdhAddEnglishCounterW(s_cpuQuery,
            L"\\Processor Information(_Total)\\% Processor Utility",
            0, &s_cpuCounter) != ERROR_SUCCESS)
    {
        PdhAddEnglishCounterW(s_cpuQuery,
            L"\\Processor(_Total)\\% Processor Time",
            0, &s_cpuCounter);
    }

    // Get CPU core count
    SYSTEM_INFO sysInfo = {};
    GetSystemInfo(&sysInfo);
    s_coreCount = static_cast<int>(sysInfo.dwNumberOfProcessors);

    // Per-core CPU counters
    s_cpuCoreCounters.resize(s_coreCount);
    for (int i = 0; i < s_coreCount; ++i)
    {
        std::wstring path = L"\\Processor Information(" + std::to_wstring(i) + L")\\% Processor Utility";
        if (PdhAddEnglishCounterW(s_cpuQuery, path.c_str(), 0, &s_cpuCoreCounters[i]) != ERROR_SUCCESS)
        {
            path = L"\\Processor(" + std::to_wstring(i) + L")\\% Processor Time";
            PdhAddEnglishCounterW(s_cpuQuery, path.c_str(), 0, &s_cpuCoreCounters[i]);
        }
    }

    // Disk I/O (PhysicalDisk _Total)
    PdhAddEnglishCounterW(s_cpuQuery,
        L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec",
        0, &s_diskReadCounter);
    PdhAddEnglishCounterW(s_cpuQuery,
        L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec",
        0, &s_diskWriteCounter);

    // Network I/O (Network Interface _Total or first interface)
    PdhAddEnglishCounterW(s_cpuQuery,
        L"\\Network Interface(*)\\Bytes Sent/sec",
        0, &s_netSentCounter);
    PdhAddEnglishCounterW(s_cpuQuery,
        L"\\Network Interface(*)\\Bytes Received/sec",
        0, &s_netRecvCounter);

    // GPU Counters (may not be available on all systems)
    PdhAddEnglishCounterW(s_cpuQuery,
        L"\\GPU Engine(*)\\Utilization Percentage",
        0, &s_gpuCounter);
    PdhAddEnglishCounterW(s_cpuQuery,
        L"\\GPU Process Memory(*)\\Dedicated Usage",
        0, &s_gpuMemCounter);

    // Prime the first sample
    PdhCollectQueryData(s_cpuQuery);
    
    // Initialize GPU vendor-specific monitoring
    GPUMonitor::Initialize();
    
    return true;
}

// ---------------------------------------------------------------------------
// GetCPUPercentPDH – returns overall CPU utilisation (0–100 %), frequency-
//                    adjusted, matching what Task Manager displays.
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetCPUPercentPDH()
{
    if (!s_cpuQuery || !s_cpuCounter) return 0.0f;

    PdhCollectQueryData(s_cpuQuery);

    PDH_FMT_COUNTERVALUE val = {};
    if (PdhGetFormattedCounterValue(s_cpuCounter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS)
        return static_cast<float>(val.doubleValue);

    return 0.0f;
}

// ---------------------------------------------------------------------------
// GetCPUPercent – kept for backward compatibility; delegates to PDH.
//                 Returns percent * 1,000,000 as an integer.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetCPUPercent()
{
    return static_cast<int>(GetCPUPercentPDH() * 1000000.0f);
}

// ---------------------------------------------------------------------------
// GetCPUCoreCount – returns the number of logical processors (threads).
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetCPUCoreCount()
{
    return s_coreCount;
}

// ---------------------------------------------------------------------------
// GetCPUCorePercent – returns CPU usage for a specific core (0–100%).
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetCPUCorePercent(int coreIndex)
{
    if (!s_cpuQuery || coreIndex < 0 || coreIndex >= s_coreCount)
        return 0.0f;

    PdhCollectQueryData(s_cpuQuery);

    PDH_FMT_COUNTERVALUE val = {};
    if (PdhGetFormattedCounterValue(s_cpuCoreCounters[coreIndex], PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS)
        return static_cast<float>(val.doubleValue);

    return 0.0f;
}

// ---------------------------------------------------------------------------
// GetCPUFrequency – returns current CPU frequency in MHz (base frequency).
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetCPUFrequency()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return 0;

    DWORD mhz = 0;
    DWORD size = sizeof(mhz);
    RegQueryValueExW(hKey, L"~MHz", nullptr, nullptr, reinterpret_cast<BYTE*>(&mhz), &size);
    RegCloseKey(hKey);

    return static_cast<int>(mhz);
}

// ---------------------------------------------------------------------------
// GetCPUName – fills buffer with CPU brand string (max 256 chars).
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetCPUName(wchar_t* buffer, int bufferSize)
{
    if (!buffer || bufferSize < 256) return false;

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    DWORD size = bufferSize * sizeof(wchar_t);
    LONG result = RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, nullptr,
                                    reinterpret_cast<BYTE*>(buffer), &size);
    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}

// ---------------------------------------------------------------------------
// GetCPUTemperature – attempts to read CPU temperature via WMI/MSR.
// NOTE: Requires administrative privileges and proper WMI namespace.
// Returns temperature in Celsius or -1.0 if unavailable.
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetCPUTemperature()
{
    // This is a placeholder - actual implementation would require:
    // 1. WMI queries (complex COM setup)
    // 2. Hardware-specific MSR reads (requires kernel driver)
    // 3. Third-party libraries like OpenHardwareMonitor
    // For now, returning -1 to indicate not available
    return -1.0f;
}

// ---------------------------------------------------------------------------
// GetDiskIOStats – returns disk read/write bytes per second.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetDiskIOStats(long long* readBytesPerSec, long long* writeBytesPerSec)
{
    if (!s_cpuQuery) return false;

    PdhCollectQueryData(s_cpuQuery);

    PDH_FMT_COUNTERVALUE valRead = {}, valWrite = {};
    
    if (s_diskReadCounter && PdhGetFormattedCounterValue(s_diskReadCounter, PDH_FMT_LARGE, nullptr, &valRead) == ERROR_SUCCESS)
        *readBytesPerSec = valRead.largeValue;
    else
        *readBytesPerSec = 0;

    if (s_diskWriteCounter && PdhGetFormattedCounterValue(s_diskWriteCounter, PDH_FMT_LARGE, nullptr, &valWrite) == ERROR_SUCCESS)
        *writeBytesPerSec = valWrite.largeValue;
    else
        *writeBytesPerSec = 0;

    return true;
}

// ---------------------------------------------------------------------------
// GetDiskSpace – returns total and free disk space in bytes for drive letter.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetDiskSpace(wchar_t driveLetter, long long* totalBytes, long long* freeBytes)
{
    wchar_t rootPath[4] = { driveLetter, L':', L'\\', L'\0' };
    
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (!GetDiskFreeSpaceExW(rootPath, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
        return false;

    *totalBytes = totalNumberOfBytes.QuadPart;
    *freeBytes = totalNumberOfFreeBytes.QuadPart;
    return true;
}

// ---------------------------------------------------------------------------
// GetNetworkStats – returns network bytes sent/received per second.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetNetworkStats(long long* bytesSentPerSec, long long* bytesRecvPerSec)
{
    if (!s_cpuQuery) return false;

    PdhCollectQueryData(s_cpuQuery);

    PDH_FMT_COUNTERVALUE valSent = {}, valRecv = {};

    if (s_netSentCounter && PdhGetFormattedCounterValue(s_netSentCounter, PDH_FMT_LARGE, nullptr, &valSent) == ERROR_SUCCESS)
        *bytesSentPerSec = valSent.largeValue;
    else
        *bytesSentPerSec = 0;

    if (s_netRecvCounter && PdhGetFormattedCounterValue(s_netRecvCounter, PDH_FMT_LARGE, nullptr, &valRecv) == ERROR_SUCCESS)
        *bytesRecvPerSec = valRecv.largeValue;
    else
        *bytesRecvPerSec = 0;

    return true;
}

// ---------------------------------------------------------------------------
// GetGPUUsagePercent – returns GPU utilization percentage (0-100).
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetGPUUsagePercent()
{
    // Try vendor-specific API first (more accurate)
    GPUMonitor::GPUMetrics metrics = {};
    if (GPUMonitor::GetGPUMetrics(metrics))
    {
        return metrics.gpuUsage;
    }

    // Fallback to PDH counter
    if (!s_cpuQuery || !s_gpuCounter) return 0.0f;

    PdhCollectQueryData(s_cpuQuery);

    PDH_FMT_COUNTERVALUE val = {};
    if (PdhGetFormattedCounterValue(s_gpuCounter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS)
        return static_cast<float>(val.doubleValue);

    return 0.0f;
}

// ---------------------------------------------------------------------------
// GetGPUTemperature – reads GPU temperature via vendor-specific APIs.
// NOTE: Requires NVAPI (NVIDIA) or ADL (AMD) to be enabled.
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetGPUTemperature()
{
    return GPUMonitor::GetTemperature();
}

// ---------------------------------------------------------------------------
// GetGPUName – returns GPU adapter description string.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetGPUName(wchar_t* buffer, int bufferSize)
{
    if (!buffer || bufferSize < 256) return false;

    IDXGIFactory* factory = nullptr;
    if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))))
        return false;

    IDXGIAdapter* adapter = nullptr;
    bool success = false;

    if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
    {
        DXGI_ADAPTER_DESC desc = {};
        if (SUCCEEDED(adapter->GetDesc(&desc)))
        {
            wcsncpy_s(buffer, bufferSize, desc.Description, _TRUNCATE);
            success = true;
        }
        adapter->Release();
    }

    factory->Release();
    return success;
}

// ---------------------------------------------------------------------------
// GetGPUClockSpeed – gets GPU core clock speed in MHz via vendor APIs.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetGPUClockSpeed()
{
    return GPUMonitor::GetCoreClock();
}

// ---------------------------------------------------------------------------
// GetGPUMemoryClock – gets GPU memory clock speed in MHz.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetGPUMemoryClock()
{
    return GPUMonitor::GetMemoryClock();
}

// ---------------------------------------------------------------------------
// GetGPUFanSpeed – gets GPU fan speed in RPM.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetGPUFanSpeed()
{
    return GPUMonitor::GetFanSpeed();
}

// ---------------------------------------------------------------------------
// GetGPUFanSpeedPercent – gets GPU fan speed as percentage (0-100).
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetGPUFanSpeedPercent()
{
    return GPUMonitor::GetFanSpeedPercent();
}

// ---------------------------------------------------------------------------
// GetGPUPowerDraw – gets GPU power consumption in Watts.
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetGPUPowerDraw()
{
    return GPUMonitor::GetPowerDraw();
}

// ---------------------------------------------------------------------------
// GetGPUVoltage – gets GPU core voltage in Volts.
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetGPUVoltage()
{
    return GPUMonitor::GetVoltage();
}

// ---------------------------------------------------------------------------
// GetGPUVendorID – gets GPU vendor identifier.
// Returns: 0x10DE (NVIDIA), 0x1002 (AMD), 0x8086 (Intel), 0 (Unknown)
// ---------------------------------------------------------------------------
__declspec(dllexport) unsigned int GetGPUVendorID()
{
    return static_cast<unsigned int>(GPUMonitor::GetVendor());
}

// ---------------------------------------------------------------------------
// GetBatteryStatus – returns battery info (percentage, charging status).
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetBatteryStatus(int* percentage, bool* isCharging, int* remainingMinutes)
{
    SYSTEM_POWER_STATUS sps = {};
    if (!GetSystemPowerStatus(&sps))
        return false;

    *percentage = (sps.BatteryLifePercent == 255) ? -1 : sps.BatteryLifePercent;
    *isCharging = (sps.ACLineStatus == 1);
    *remainingMinutes = (sps.BatteryLifeTime == (DWORD)-1) ? -1 : static_cast<int>(sps.BatteryLifeTime / 60);

    return true;
}

// ---------------------------------------------------------------------------
// GetSystemUptime – returns system uptime in seconds.
// ---------------------------------------------------------------------------
__declspec(dllexport) long long GetSystemUptime()
{
    return GetTickCount64() / 1000;
}

// ---------------------------------------------------------------------------
// GetProcessCount – returns number of running processes.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetProcessCount()
{
    DWORD processes[1024], bytesReturned;
    if (!EnumProcesses(processes, sizeof(processes), &bytesReturned))
        return 0;

    return bytesReturned / sizeof(DWORD);
}

// ---------------------------------------------------------------------------
// GetThreadCount – returns total number of threads across all processes.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetThreadCount()
{
    PERFORMANCE_INFORMATION perfInfo = { sizeof(perfInfo) };
    if (!GetPerformanceInfo(&perfInfo, sizeof(perfInfo)))
        return 0;

    return static_cast<int>(perfInfo.ThreadCount);
}

// ---------------------------------------------------------------------------
// GetHandleCount – returns total number of handles across all processes.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetHandleCount()
{
    PERFORMANCE_INFORMATION perfInfo = { sizeof(perfInfo) };
    if (!GetPerformanceInfo(&perfInfo, sizeof(perfInfo)))
        return 0;

    return static_cast<int>(perfInfo.HandleCount);
}

// ---------------------------------------------------------------------------
// GetPageFileUsage – returns page file (virtual memory) usage info.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetPageFileUsage(long long* totalBytes, long long* usedBytes)
{
    PERFORMANCE_INFORMATION perfInfo = { sizeof(perfInfo) };
    if (!GetPerformanceInfo(&perfInfo, sizeof(perfInfo)))
        return false;

    *totalBytes = static_cast<long long>(perfInfo.CommitLimit) * perfInfo.PageSize;
    *usedBytes = static_cast<long long>(perfInfo.CommitTotal) * perfInfo.PageSize;

    return true;
}

// ---------------------------------------------------------------------------
// GetMotherboardTemperature – placeholder for motherboard temp reading.
// ---------------------------------------------------------------------------
__declspec(dllexport) float GetMotherboardTemperature()
{
    // Requires WMI or hardware monitoring libraries
    return -1.0f;
}

// ---------------------------------------------------------------------------
// GetFanSpeed – placeholder for fan speed in RPM.
// ---------------------------------------------------------------------------
__declspec(dllexport) int GetFanSpeed(int fanIndex)
{
    // Requires hardware monitoring libraries
    return -1;
}

// ---------------------------------------------------------------------------
// GetSystemInfo – fills a buffer with system information.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetSystemInfoString(wchar_t* buffer, int bufferSize)
{
    if (!buffer || bufferSize < 512) return false;

    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    
    // Get Windows version
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll)
    {
        typedef LONG(WINAPI* RtlGetVersionPtr)(OSVERSIONINFOEXW*);
        RtlGetVersionPtr RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (RtlGetVersion)
            RtlGetVersion(&osvi);
    }

    swprintf_s(buffer, bufferSize,
        L"Windows %d.%d Build %d",
        osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

    return true;
}

// ---------------------------------------------------------------------------
// GetMemoryInfo – physical RAM via GlobalMemoryStatusEx;
//                 VRAM via DXGI (IDXGIAdapter3 for accurate used/budget).
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetMemoryInfo(long long* memTotal, long long* memUsed,
                                          long long* vMemTotal, long long* vMemUsed)
{
    // Physical RAM
    MEMORYSTATUSEX ms = { sizeof(ms) };
    if (!GlobalMemoryStatusEx(&ms)) return false;
    *memTotal = static_cast<long long>(ms.ullTotalPhys);
    *memUsed  = static_cast<long long>(ms.ullTotalPhys - ms.ullAvailPhys);

    // GPU VRAM (primary adapter)
    *vMemTotal = 0;
    *vMemUsed  = 0;

    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)))) {
        IDXGIAdapter* adapter = nullptr;
        if (SUCCEEDED(factory->EnumAdapters(0, &adapter))) {
            // Prefer IDXGIAdapter3 (Windows 8.1+) for live usage query
            IDXGIAdapter3* adapter3 = nullptr;
            if (SUCCEEDED(adapter->QueryInterface(__uuidof(IDXGIAdapter3),
                                                   reinterpret_cast<void**>(&adapter3)))) {
                DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
                if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(
                        0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
                    *vMemTotal = static_cast<long long>(info.Budget);
                    *vMemUsed  = static_cast<long long>(info.CurrentUsage);
                }
                adapter3->Release();
            } else {
                // Fallback: static capacity from adapter description
                DXGI_ADAPTER_DESC desc = {};
                if (SUCCEEDED(adapter->GetDesc(&desc)))
                    *vMemTotal = static_cast<long long>(desc.DedicatedVideoMemory);
            }
            adapter->Release();
        }
        factory->Release();
    }
    return true;
}

// ---------------------------------------------------------------------------
// GetJumboIcon – renders the 256×256 shell icon for the given path into a
//                static buffer and returns a pointer to raw 32-bpp BGRA pixel
//                data (256×256×4 = 262144 bytes, top-down).
// ---------------------------------------------------------------------------
__declspec(dllexport) BYTE* GetJumboIcon(const wchar_t* path, int* bmpLength)
{
    *bmpLength = 0;

    // Get the system image list at SHIL_JUMBO (256×256)
    IImageList* imageList = nullptr;
    if (FAILED(SHGetImageList(SHIL_JUMBO, IID_IImageList,
                               reinterpret_cast<void**>(&imageList))))
        return nullptr;

    // Resolve the shell icon index for this path
    SHFILEINFOW sfi = {};
    SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);

    HICON hIcon = nullptr;
    imageList->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon);
    imageList->Release();

    if (!hIcon) return nullptr;

    // Render icon into a 32-bpp top-down DIB
    const int SIZE = 256;
    const int BYTES = SIZE * SIZE * 4;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       =  SIZE;
    bmi.bmiHeader.biHeight      = -SIZE; // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC     screenDC = GetDC(nullptr);
    HDC     memDC    = CreateCompatibleDC(screenDC);
    VOID*   bits     = nullptr;
    HBITMAP hBmp     = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, screenDC);

    if (!hBmp) {
        DestroyIcon(hIcon);
        DeleteDC(memDC);
        return nullptr;
    }

    HGDIOBJ oldBmp = SelectObject(memDC, hBmp);
    RECT rc = { 0, 0, SIZE, SIZE };
    FillRect(memDC, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    DrawIconEx(memDC, 0, 0, hIcon, SIZE, SIZE, 0, nullptr, DI_NORMAL);
    SelectObject(memDC, oldBmp);
    DeleteDC(memDC);
    DestroyIcon(hIcon);

    // Copy pixels into our static buffer and release the GDI bitmap
    s_iconBuf.resize(BYTES);
    memcpy(s_iconBuf.data(), bits, BYTES);
    DeleteObject(hBmp);

    *bmpLength = BYTES;
    return s_iconBuf.data();
}

// ---------------------------------------------------------------------------
// SimulateKeypress – sends a virtual key down+up pair via SendInput.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool SimulateKeypress(unsigned int keyCode)
{
    INPUT inputs[2] = {};
    inputs[0].type       = INPUT_KEYBOARD;
    inputs[0].ki.wVk     = static_cast<WORD>(keyCode);
    inputs[1].type       = INPUT_KEYBOARD;
    inputs[1].ki.wVk     = static_cast<WORD>(keyCode);
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(2, inputs, sizeof(INPUT)) == 2;
}

// ---------------------------------------------------------------------------
// GetAvatarPath – fills buffer with the ASCII path of the current user's
//                 account picture (the one shown in Windows Start menu).
//                 Returns false if none is found.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetAvatarPath(const wchar_t* /*username*/, BYTE* buffer)
{
    std::wstring avatarPath;
    
    // Method 1: Check HKEY_LOCAL_MACHINE registry (where Windows actually stores avatar paths)
    // Path: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\AccountPicture\Users\{SID}
    HKEY hKeyUsers = nullptr;
    
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AccountPicture\\Users",
        0, KEY_READ, &hKeyUsers) == ERROR_SUCCESS)
    {
        // Enumerate subkeys to find SIDs
        DWORD index = 0;
        wchar_t sidString[256] = {};
        DWORD sidSize = sizeof(sidString) / sizeof(wchar_t);
        
        // Get the first SID (usually the current user if only one account)
        // In a multi-user scenario, you'd need to match against current user's SID
        while (RegEnumKeyExW(hKeyUsers, index, sidString, &sidSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
        {
            std::wstring sidKeyPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AccountPicture\\Users\\";
            sidKeyPath += sidString;
            
            HKEY hKeySID = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sidKeyPath.c_str(), 0, KEY_READ, &hKeySID) == ERROR_SUCCESS)
            {
                wchar_t imagePath[MAX_PATH] = {};
                DWORD size = sizeof(imagePath);
                DWORD type = REG_SZ;
                
                // Try different image sizes (prefer larger, as shown in your registry)
                const wchar_t* imageKeys[] = { 
                    L"Image1080", L"Image448", L"Image240", L"Image208", L"Image192", 
                    L"Image96", L"Image64", L"Image48", L"Image40", L"Image32", L"Image424"
                };
                
                for (const auto& key : imageKeys)
                {
                    size = sizeof(imagePath);
                    if (RegQueryValueExW(hKeySID, key, nullptr, &type, 
                        reinterpret_cast<BYTE*>(imagePath), &size) == ERROR_SUCCESS)
                    {
                        // Verify file exists
                        DWORD attrib = GetFileAttributesW(imagePath);
                        if (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY))
                        {
                            avatarPath = imagePath;
                            break;
                        }
                    }
                }
                
                RegCloseKey(hKeySID);
                
                if (!avatarPath.empty())
                {
                    RegCloseKey(hKeyUsers);
                    int len = WideCharToMultiByte(CP_ACP, 0, avatarPath.c_str(), -1,
                                                 reinterpret_cast<char*>(buffer), 2048, nullptr, nullptr);
                    return len > 0;
                }
            }
            
            // Reset for next iteration
            index++;
            sidSize = sizeof(sidString) / sizeof(wchar_t);
        }
        
        RegCloseKey(hKeyUsers);
    }
    
    // Method 2: Direct path to Public\AccountPictures with SID-based filename
    // Based on your screenshot: C:\Users\Public\AccountPictures\S-1-5-21-...-Image1080.jpg
    PWSTR publicFolder = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Public, 0, nullptr, &publicFolder)))
    {
        std::wstring publicPath = std::wstring(publicFolder) + L"\\AccountPictures\\";
        CoTaskMemFree(publicFolder);
        
        // Search for any image files in the AccountPictures folder
        WIN32_FIND_DATAW findData = {};
        std::wstring searchPattern = publicPath + L"*-Image*.jpg";
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE)
        {
            // Use the first found image (most likely the current user's avatar)
            avatarPath = publicPath + findData.cFileName;
            FindClose(hFind);
            
            int len = WideCharToMultiByte(CP_ACP, 0, avatarPath.c_str(), -1,
                                         reinterpret_cast<char*>(buffer), 2048, nullptr, nullptr);
            return len > 0;
        }
        
        // Also try PNG files
        searchPattern = publicPath + L"*-Image*.png";
        hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE)
        {
            avatarPath = publicPath + findData.cFileName;
            FindClose(hFind);
            
            int len = WideCharToMultiByte(CP_ACP, 0, avatarPath.c_str(), -1,
                                         reinterpret_cast<char*>(buffer), 2048, nullptr, nullptr);
            return len > 0;
        }
    }
    
    // Method 3: Fallback to default user picture
    PWSTR programData = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &programData)))
    {
        std::wstring defaultPicPath = std::wstring(programData) + 
            L"\\Microsoft\\User Account Pictures\\user.png";
        CoTaskMemFree(programData);
        
        DWORD attrib = GetFileAttributesW(defaultPicPath.c_str());
        if (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            int len = WideCharToMultiByte(CP_ACP, 0, defaultPicPath.c_str(), -1,
                                         reinterpret_cast<char*>(buffer), 2048, nullptr, nullptr);
            return len > 0;
        }
    }
    
    return false;
}

// ---------------------------------------------------------------------------
// SetMouseHook – installs a WH_MOUSE_LL hook on a dedicated background thread
//                (global hooks require a message pump on the installing thread).
// ---------------------------------------------------------------------------
__declspec(dllexport) bool SetMouseHook(HINSTANCE /*hMod*/)
{
    if (s_hookRunning) return true;

    std::thread(HookThreadProc, s_hModule).detach();

    // Wait up to 500 ms for the hook to install
    for (int i = 0; i < 50 && !s_hookRunning; ++i)
        Sleep(10);

    return s_hookRunning.load();
}

// ---------------------------------------------------------------------------
// UnhookMouseHook – signals the hook thread to exit.
// ---------------------------------------------------------------------------
__declspec(dllexport) bool UnhookMouseHook()
{
    s_hookRunning = false;
    if (s_hookThreadId)
        PostThreadMessage(s_hookThreadId, WM_QUIT, 0, 0);
    return true;
}

// ---------------------------------------------------------------------------
// GetMouseDown / GetMouseUp – returns true once per event (cleared on read).
// ---------------------------------------------------------------------------
__declspec(dllexport) bool GetMouseDown() { return s_mouseDown.exchange(false); }
__declspec(dllexport) bool GetMouseUp()   { return s_mouseUp.exchange(false);   }

} // extern "C"


// ── DllMain ────────────────────────────────────────────────────────────────
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*reserved*/)
{
    if (reason == DLL_PROCESS_ATTACH) {
        s_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
    } else if (reason == DLL_PROCESS_DETACH) {
        UnhookMouseHook();
        if (s_cpuQuery) {
            PdhCloseQuery(s_cpuQuery);
            s_cpuQuery = nullptr;
        }
        GPUMonitor::Shutdown();
    }
    return TRUE;
}
