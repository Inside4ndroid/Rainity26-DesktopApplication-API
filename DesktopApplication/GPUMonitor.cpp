#include "GPUMonitor.h"
#include <dxgi.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>

#pragma comment(lib, "dxgi.lib")

// ═══════════════════════════════════════════════════════════════════════════
// NVIDIA NVAPI Support
// ═══════════════════════════════════════════════════════════════════════════
// To enable NVIDIA support:
// 1. Download NVAPI SDK from https://developer.nvidia.com/nvapi
// 2. Copy THE ENTIRE include folder contents to this directory
//    (not just nvapi.h, but ALL header files!)
// 3. Add nvapi64.lib (or nvapi.lib) to linker
// 4. Uncomment the define below
// ═══════════════════════════════════════════════════════════════════════════

#define ENABLE_NVIDIA_NVAPI

#ifdef ENABLE_NVIDIA_NVAPI
#include "nvapi.h"
#pragma comment(lib, "nvapi64.lib")
#endif

// ═══════════════════════════════════════════════════════════════════════════
// AMD ADL Support
// ═══════════════════════════════════════════════════════════════════════════
// To enable AMD support:
// 1. Download ADL SDK from https://github.com/GPUOpen-LibrariesAndSDKs/display-library
// 2. Add adl_sdk.h, adl_structures.h, adl_defines.h to this directory
// 3. Uncomment the define below
// ═══════════════════════════════════════════════════════════════════════════

#define ENABLE_AMD_ADL

#ifdef ENABLE_AMD_ADL
#include "adl_sdk.h"
#include "adl_structures.h"
#include "adl_defines.h"
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Internal State
// ═══════════════════════════════════════════════════════════════════════════

namespace GPUMonitor
{
    namespace Internal
    {
        static bool g_initialized = false;
        static GPUVendor g_vendor = GPUVendor::Unknown;
        static GPUInfo g_gpuInfo = {};
        
#ifdef ENABLE_NVIDIA_NVAPI
        static NvPhysicalGpuHandle g_nvGPU = nullptr;
        
        // nvidia-smi cache for power/voltage
        struct NvidiaSmiCache
        {
            float powerDraw = -1.0f;
            float powerLimit = -1.0f;
            float voltage = -1.0f;
            DWORD lastUpdate = 0;
            const DWORD cacheLifetime = 1000; // Cache for 1 second
        };
        static NvidiaSmiCache g_smiCache;
        
        // Execute nvidia-smi and capture output
        std::string ExecuteNvidiaSmi(const std::string& arguments)
        {
            SECURITY_ATTRIBUTES sa = {};
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = nullptr;

            HANDLE hReadPipe = nullptr;
            HANDLE hWritePipe = nullptr;
            if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
                return "";

            SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOA si = {};
            si.cb = sizeof(STARTUPINFOA);
            si.hStdOutput = hWritePipe;
            si.hStdError = hWritePipe;
            si.dwFlags |= STARTF_USESTDHANDLES;

            PROCESS_INFORMATION pi = {};

            // Try to find nvidia-smi.exe
            std::string cmdLine = "nvidia-smi " + arguments;
            
            if (!CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()), 
                nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
            {
                CloseHandle(hWritePipe);
                CloseHandle(hReadPipe);
                return "";
            }

            CloseHandle(hWritePipe);

            // Read output
            std::string output;
            char buffer[4096];
            DWORD bytesRead = 0;

            while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                output += buffer;
            }

            WaitForSingleObject(pi.hProcess, 5000); // Wait max 5 seconds

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);

            return output;
        }

        // Parse nvidia-smi output for power and voltage
        void UpdateNvidiaSmiData()
        {
            DWORD currentTime = GetTickCount();
            
            // Check if cache is still valid
            if (g_smiCache.lastUpdate > 0 && 
                (currentTime - g_smiCache.lastUpdate) < g_smiCache.cacheLifetime)
            {
                return; // Use cached data
            }

            // Query power.draw, power.limit, and voltage
            std::string output = ExecuteNvidiaSmi(
                "--query-gpu=power.draw,power.limit,clocks.current.graphics "
                "--format=csv,noheader,nounits -i 0"
            );

            if (!output.empty())
            {
                // Parse CSV output: power.draw, power.limit, clock
                std::istringstream iss(output);
                std::string token;
                int field = 0;

                while (std::getline(iss, token, ','))
                {
                    // Trim whitespace
                    token.erase(0, token.find_first_not_of(" \t\r\n"));
                    token.erase(token.find_last_not_of(" \t\r\n") + 1);

                    if (field == 0) // power.draw
                    {
                        // Check for N/A or [N/A]
                        if (token.find("N/A") == std::string::npos)
                        {
                            try {
                                g_smiCache.powerDraw = std::stof(token);
                            } catch (...) {
                                g_smiCache.powerDraw = -1.0f;
                            }
                        }
                        else
                        {
                            g_smiCache.powerDraw = -1.0f; // N/A means not available
                        }
                    }
                    else if (field == 1) // power.limit
                    {
                        if (token.find("N/A") == std::string::npos)
                        {
                            try {
                                g_smiCache.powerLimit = std::stof(token);
                            } catch (...) {
                                g_smiCache.powerLimit = -1.0f;
                            }
                        }
                        else
                        {
                            g_smiCache.powerLimit = -1.0f;
                        }
                    }

                    field++;
                }

                g_smiCache.lastUpdate = currentTime;
            }
        }
#endif

#ifdef ENABLE_AMD_ADL
        static void* g_adlContext = nullptr;
        static int g_adlAdapterIndex = 0;
#endif

        // ═══════════════════════════════════════════════════════════════════
        // Helper Functions
        // ═══════════════════════════════════════════════════════════════════

        GPUVendor DetectGPUVendor()
        {
            IDXGIFactory* factory = nullptr;
            if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))))
                return GPUVendor::Unknown;

            IDXGIAdapter* adapter = nullptr;
            if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
            {
                DXGI_ADAPTER_DESC desc = {};
                if (SUCCEEDED(adapter->GetDesc(&desc)))
                {
                    g_gpuInfo.vendorId = desc.VendorId;
                    g_gpuInfo.deviceId = desc.DeviceId;
                    g_gpuInfo.name = desc.Description;

                    adapter->Release();
                    factory->Release();

                    // Detect vendor by VendorId
                    if (desc.VendorId == static_cast<unsigned int>(GPUVendor::NVIDIA))
                        return GPUVendor::NVIDIA;
                    else if (desc.VendorId == static_cast<unsigned int>(GPUVendor::AMD))
                        return GPUVendor::AMD;
                    else if (desc.VendorId == static_cast<unsigned int>(GPUVendor::Intel))
                        return GPUVendor::Intel;
                }
                adapter->Release();
            }
            factory->Release();

            return GPUVendor::Unknown;
        }

#ifdef ENABLE_NVIDIA_NVAPI
        bool InitializeNVAPI()
        {
            if (NvAPI_Initialize() != NVAPI_OK)
                return false;

            NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = {};
            NvU32 gpuCount = 0;

            if (NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount) != NVAPI_OK || gpuCount == 0)
            {
                NvAPI_Unload();
                return false;
            }

            g_nvGPU = gpuHandles[0]; // Use first GPU
            return true;
        }
#endif

#ifdef ENABLE_AMD_ADL
        // ADL Memory allocation callback
        static void* __stdcall ADL_Main_Memory_Alloc(int iSize)
        {
            void* lpBuffer = malloc(iSize);
            return lpBuffer;
        }

        static void __stdcall ADL_Main_Memory_Free(void** lpBuffer)
        {
            if (*lpBuffer != nullptr)
            {
                free(*lpBuffer);
                *lpBuffer = nullptr;
            }
        }

        // ADL function pointers
        typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
        typedef int (*ADL_MAIN_CONTROL_DESTROY)();
        typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
        typedef int (*ADL_OVERDRIVE5_TEMPERATURE_GET)(int, int, ADLTemperature*);
        typedef int (*ADL_OVERDRIVE5_FANSPEED_GET)(int, int, ADLFanSpeedValue*);
        typedef int (*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int, ADLPMActivity*);

        static HMODULE g_adlDll = nullptr;
        static ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create = nullptr;

        bool InitializeADL()
        {
            g_adlDll = LoadLibraryA("atiadlxx.dll");
            if (!g_adlDll)
                g_adlDll = LoadLibraryA("atiadlxy.dll");

            if (!g_adlDll)
                return false;

            ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(g_adlDll, "ADL_Main_Control_Create");
            if (!ADL_Main_Control_Create)
                return false;

            if (ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1) != ADL_OK)
                return false;

            return true;
        }
#endif

    } // namespace Internal

    // ═══════════════════════════════════════════════════════════════════════
    // Public API Implementation
    // ═══════════════════════════════════════════════════════════════════════

    bool Initialize()
    {
        if (Internal::g_initialized)
            return true;

        // Detect GPU vendor
        Internal::g_vendor = Internal::DetectGPUVendor();
        Internal::g_gpuInfo.vendor = Internal::g_vendor;

        bool apiInitialized = false;

        switch (Internal::g_vendor)
        {
        case GPUVendor::NVIDIA:
#ifdef ENABLE_NVIDIA_NVAPI
            apiInitialized = Internal::InitializeNVAPI();
#endif
            break;

        case GPUVendor::AMD:
#ifdef ENABLE_AMD_ADL
            apiInitialized = Internal::InitializeADL();
#endif
            break;

        case GPUVendor::Intel:
            // Intel uses WMI/built-in APIs
            apiInitialized = true;
            break;

        default:
            break;
        }

        Internal::g_gpuInfo.monitoringAvailable = apiInitialized;
        Internal::g_initialized = true;

        return true;
    }

    void Shutdown()
    {
        if (!Internal::g_initialized)
            return;

#ifdef ENABLE_NVIDIA_NVAPI
        if (Internal::g_vendor == GPUVendor::NVIDIA)
        {
            NvAPI_Unload();
        }
#endif

#ifdef ENABLE_AMD_ADL
        if (Internal::g_vendor == GPUVendor::AMD)
        {
            // ADL cleanup
        }
#endif

        Internal::g_initialized = false;
    }

    bool GetGPUInfo(GPUInfo& info)
    {
        if (!Internal::g_initialized)
            return false;

        info = Internal::g_gpuInfo;
        return true;
    }

    bool GetGPUMetrics(GPUMetrics& metrics)
    {
        if (!Internal::g_initialized)
            return false;

        // Initialize structure
        metrics = {};

        switch (Internal::g_vendor)
        {
        case GPUVendor::NVIDIA:
#ifdef ENABLE_NVIDIA_NVAPI
            {
                // Temperature
                NV_GPU_THERMAL_SETTINGS thermal = {};
                thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
                if (NvAPI_GPU_GetThermalSettings(Internal::g_nvGPU, NVAPI_THERMAL_TARGET_ALL, &thermal) == NVAPI_OK)
                {
                    if (thermal.count > 0)
                    {
                        metrics.temperature = static_cast<float>(thermal.sensor[0].currentTemp);
                        metrics.temperatureAvailable = true;
                    }
                }

                // Clock Speeds
                NV_GPU_CLOCK_FREQUENCIES clocks = {};
                clocks.version = NV_GPU_CLOCK_FREQUENCIES_VER;
                clocks.ClockType = NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ;
                if (NvAPI_GPU_GetAllClockFrequencies(Internal::g_nvGPU, &clocks) == NVAPI_OK)
                {
                    metrics.gpuClockCurrent = static_cast<int>(clocks.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency / 1000);
                    metrics.memoryClockCurrent = static_cast<int>(clocks.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency / 1000);
                    metrics.clocksAvailable = true;
                }

                // GPU Usage (Dynamic Performance States)
                NV_GPU_DYNAMIC_PSTATES_INFO_EX pstatesInfo = {};
                pstatesInfo.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
                if (NvAPI_GPU_GetDynamicPstatesInfoEx(Internal::g_nvGPU, &pstatesInfo) == NVAPI_OK)
                {
                    // GPU utilization is in domain 0 (GPU)
                    if (pstatesInfo.utilization[0].bIsPresent)
                    {
                        metrics.gpuUsage = static_cast<float>(pstatesInfo.utilization[0].percentage);
                    }
                }

                // Fan Speed (using tachometer reading)
                NvU32 fanSpeed = 0;
                if (NvAPI_GPU_GetTachReading(Internal::g_nvGPU, &fanSpeed) == NVAPI_OK)
                {
                    metrics.fanSpeedRPM = static_cast<int>(fanSpeed);
                    metrics.fanAvailable = true;
                    
                    // Calculate percentage (assume max 3000 RPM for now, can be improved)
                    metrics.fanSpeedPercent = static_cast<int>((fanSpeed * 100) / 3000);
                    if (metrics.fanSpeedPercent > 100) metrics.fanSpeedPercent = 100;
                }

                // Power and Voltage via nvidia-smi
                Internal::UpdateNvidiaSmiData();
                
                if (Internal::g_smiCache.powerDraw >= 0.0f)
                {
                    metrics.powerDraw = Internal::g_smiCache.powerDraw;
                    metrics.powerLimit = Internal::g_smiCache.powerLimit;
                    metrics.powerAvailable = true;
                }
                else
                {
                    metrics.powerAvailable = false;
                }

                // Voltage - not available via nvidia-smi standard queries
                // Would require additional parsing or sensor access
                metrics.voltageAvailable = false;

                return true;
            }
#endif
            break;

        case GPUVendor::AMD:
#ifdef ENABLE_AMD_ADL
            {
                // AMD ADL implementation
                // Temperature, clocks, fan speed via ADL APIs
                return false; // Placeholder
            }
#endif
            break;

        case GPUVendor::Intel:
            // Intel has limited monitoring capabilities
            // Use WMI or built-in APIs
            break;

        default:
            break;
        }

        return false;
    }

    float GetTemperature()
    {
        GPUMetrics metrics = {};
        if (GetGPUMetrics(metrics) && metrics.temperatureAvailable)
            return metrics.temperature;
        return -1.0f;
    }

    int GetCoreClock()
    {
        GPUMetrics metrics = {};
        if (GetGPUMetrics(metrics) && metrics.clocksAvailable)
            return metrics.gpuClockCurrent;
        return 0;
    }

    int GetMemoryClock()
    {
        GPUMetrics metrics = {};
        if (GetGPUMetrics(metrics) && metrics.clocksAvailable)
            return metrics.memoryClockCurrent;
        return 0;
    }

    int GetFanSpeed()
    {
        GPUMetrics metrics = {};
        if (GetGPUMetrics(metrics) && metrics.fanAvailable)
            return metrics.fanSpeedRPM;
        return -1;
    }

    int GetFanSpeedPercent()
    {
        GPUMetrics metrics = {};
        if (GetGPUMetrics(metrics) && metrics.fanAvailable)
            return metrics.fanSpeedPercent;
        return -1;
    }

    float GetPowerDraw()
    {
        GPUMetrics metrics = {};
        if (GetGPUMetrics(metrics) && metrics.powerAvailable)
            return metrics.powerDraw;
        return -1.0f;
    }

    float GetVoltage()
    {
        GPUMetrics metrics = {};
        if (GetGPUMetrics(metrics) && metrics.voltageAvailable)
            return metrics.coreVoltage;
        return -1.0f;
    }

    GPUVendor GetVendor()
    {
        return Internal::g_vendor;
    }

} // namespace GPUMonitor
