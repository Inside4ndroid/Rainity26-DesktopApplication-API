# 📊 System Monitor API - Comprehensive Windows Monitoring DLL

A complete C++ DLL library for monitoring all aspects of Windows system performance, hardware statistics, and resource usage.

## 🚀 Features

### 💻 CPU Monitoring
- **Overall CPU Usage**: Frequency-adjusted utilization matching Task Manager
- **Per-Core Usage**: Monitor individual CPU core/thread utilization
- **CPU Information**: Brand name, core count, base frequency
- **Temperature Monitoring**: Placeholder for temperature readings (requires hardware-specific implementation)

### 🧠 Memory Monitoring
- **Physical RAM**: Total and used system memory
- **GPU VRAM**: Video memory usage via DXGI (Windows 8.1+)
- **Page File**: Virtual memory usage statistics

### 💾 Disk Monitoring
- **Disk I/O**: Real-time read/write throughput (bytes/sec)
- **Disk Space**: Total and free space per drive letter

### 🌐 Network Monitoring
- **Network Throughput**: Bytes sent/received per second across all interfaces

### 🎮 GPU Monitoring
- **GPU Usage**: Utilization percentage
- **GPU Information**: Adapter name/description
- **VRAM Usage**: Dedicated video memory tracking
- **Temperature & Clock Speed**: Placeholders for vendor-specific implementations

### 🔋 Battery & Power
- **Battery Status**: Charge percentage, charging state, remaining time
- **Power Information**: AC line status

### ℹ️ System Information
- **System Uptime**: Time since last boot
- **Process Statistics**: Process, thread, and handle counts
- **Windows Version**: OS version and build number

### 🖱️ Input & UI Utilities
- **Mouse Hooks**: Low-level mouse event monitoring
- **Keyboard Simulation**: Send virtual key presses
- **Icon Rendering**: Extract 256×256 shell icons for files/folders
- **User Avatar**: Retrieve current user's account picture path

## 📋 Requirements

- **Windows 8.1 or later** (for DXGI 1.4 features)
- **C++17 compiler** (Visual Studio 2017+)
- **Administrator privileges** (optional, for some advanced features)

### ⚙️ Dependencies
- `pdh.lib` - Performance Data Helper
- `dxgi.lib` - DirectX Graphics Infrastructure
- `powrprof.lib` - Power management
- `setupapi.lib` - Device setup
- `iphlpapi.lib` - IP Helper
- `psapi.lib` - Process status
- `shell32.lib`, `comctl32.lib`, `user32.lib`, `gdi32.lib`

## 💡 Usage

### 1️⃣ Initialize the Library

```cpp
if (!Initialize()) {
    // Handle initialization failure
    return;
}
```

### 2️⃣ Monitor CPU

```cpp
// Overall CPU usage
float cpuUsage = GetCPUPercentPDH();
printf("CPU Usage: %.2f%%\n", cpuUsage);

// Per-core usage
int coreCount = GetCPUCoreCount();
for (int i = 0; i < coreCount; i++) {
    float coreUsage = GetCPUCorePercent(i);
    printf("Core %d: %.2f%%\n", i, coreUsage);
}

// CPU information
wchar_t cpuName[256];
if (GetCPUName(cpuName, 256)) {
    wprintf(L"CPU: %s\n", cpuName);
}
int frequency = GetCPUFrequency();
printf("Base Frequency: %d MHz\n", frequency);
```

### 3️⃣ Monitor Memory

```cpp
long long memTotal, memUsed, vMemTotal, vMemUsed;
if (GetMemoryInfo(&memTotal, &memUsed, &vMemTotal, &vMemUsed)) {
    printf("RAM: %.2f GB / %.2f GB\n", 
           memUsed / 1073741824.0, 
           memTotal / 1073741824.0);
    printf("VRAM: %.2f GB / %.2f GB\n", 
           vMemUsed / 1073741824.0, 
           vMemTotal / 1073741824.0);
}

long long pageTotal, pageUsed;
if (GetPageFileUsage(&pageTotal, &pageUsed)) {
    printf("Page File: %.2f GB / %.2f GB\n",
           pageUsed / 1073741824.0,
           pageTotal / 1073741824.0);
}
```

### 4️⃣ Monitor Disk I/O

```cpp
long long readBytes, writeBytes;
if (GetDiskIOStats(&readBytes, &writeBytes)) {
    printf("Disk Read: %.2f MB/s\n", readBytes / 1048576.0);
    printf("Disk Write: %.2f MB/s\n", writeBytes / 1048576.0);
}

long long totalSpace, freeSpace;
if (GetDiskSpace(L'C', &totalSpace, &freeSpace)) {
    printf("C: Drive: %.2f GB free of %.2f GB\n",
           freeSpace / 1073741824.0,
           totalSpace / 1073741824.0);
}
```

### 5️⃣ Monitor Network

```cpp
long long bytesSent, bytesRecv;
if (GetNetworkStats(&bytesSent, &bytesRecv)) {
    printf("Network Upload: %.2f MB/s\n", bytesSent / 1048576.0);
    printf("Network Download: %.2f MB/s\n", bytesRecv / 1048576.0);
}
```

### 6️⃣ Monitor GPU

```cpp
float gpuUsage = GetGPUUsagePercent();
printf("GPU Usage: %.2f%%\n", gpuUsage);

wchar_t gpuName[256];
if (GetGPUName(gpuName, 256)) {
    wprintf(L"GPU: %s\n", gpuName);
}
```

### 7️⃣ Check Battery Status

```cpp
int percentage;
bool isCharging;
int remainingMinutes;
if (GetBatteryStatus(&percentage, &isCharging, &remainingMinutes)) {
    if (percentage >= 0) {
        printf("Battery: %d%% %s\n", 
               percentage, 
               isCharging ? "(Charging)" : "");
        if (remainingMinutes >= 0) {
            printf("Time Remaining: %d hours %d minutes\n",
                   remainingMinutes / 60,
                   remainingMinutes % 60);
        }
    } else {
        printf("No battery detected\n");
    }
}
```

### 8️⃣ System Information

```cpp
long long uptime = GetSystemUptime();
printf("System Uptime: %lld hours %lld minutes\n", 
       uptime / 3600, 
       (uptime % 3600) / 60);

int processCount = GetProcessCount();
int threadCount = GetThreadCount();
int handleCount = GetHandleCount();
printf("Processes: %d, Threads: %d, Handles: %d\n",
       processCount, threadCount, handleCount);

wchar_t sysInfo[512];
if (GetSystemInfoString(sysInfo, 512)) {
    wprintf(L"System: %s\n", sysInfo);
}
```

## 💻 C# Interop Example

```csharp
using System;
using System.Runtime.InteropServices;

public class SystemMonitor
{
    const string DLL_NAME = "DesktopApplication.dll";

    [DllImport(DLL_NAME)]
    public static extern bool Initialize();

    [DllImport(DLL_NAME)]
    public static extern float GetCPUPercentPDH();

    [DllImport(DLL_NAME)]
    public static extern bool GetMemoryInfo(
        out long memTotal, out long memUsed,
        out long vMemTotal, out long vMemUsed);

    [DllImport(DLL_NAME)]
    public static extern int GetCPUCoreCount();

    [DllImport(DLL_NAME)]
    public static extern float GetCPUCorePercent(int coreIndex);

    [DllImport(DLL_NAME)]
    public static extern bool GetDiskIOStats(
        out long readBytesPerSec, 
        out long writeBytesPerSec);

    [DllImport(DLL_NAME)]
    public static extern bool GetNetworkStats(
        out long bytesSentPerSec, 
        out long bytesRecvPerSec);

    [DllImport(DLL_NAME)]
    public static extern float GetGPUUsagePercent();

    [DllImport(DLL_NAME, CharSet = CharSet.Unicode)]
    public static extern bool GetGPUName(
        [MarshalAs(UnmanagedType.LPWStr)] StringBuilder buffer, 
        int bufferSize);

    // Usage example
    public static void Main()
    {
        if (!Initialize())
        {
            Console.WriteLine("Failed to initialize");
            return;
        }

        float cpuUsage = GetCPUPercentPDH();
        Console.WriteLine($"CPU Usage: {cpuUsage:F2}%");

        GetMemoryInfo(out long memTotal, out long memUsed,
                      out long vMemTotal, out long vMemUsed);
        Console.WriteLine($"RAM: {memUsed / 1073741824.0:F2} GB / {memTotal / 1073741824.0:F2} GB");
    }
}
```

## 🎮 Unity Integration Example

```csharp
using UnityEngine;
using System.Runtime.InteropServices;
using System.Text;

public class SystemMonitorUnity : MonoBehaviour
{
    const string DLL_NAME = "DesktopApplication";

    [DllImport(DLL_NAME)]
    private static extern bool Initialize();

    [DllImport(DLL_NAME)]
    private static extern float GetCPUPercentPDH();

    [DllImport(DLL_NAME)]
    private static extern bool GetMemoryInfo(
        out long memTotal, out long memUsed,
        out long vMemTotal, out long vMemUsed);

    void Start()
    {
        if (!Initialize())
        {
            Debug.LogError("Failed to initialize System Monitor");
            return;
        }
    }

    void Update()
    {
        float cpuUsage = GetCPUPercentPDH();
        Debug.Log($"CPU: {cpuUsage:F1}%");

        GetMemoryInfo(out long memTotal, out long memUsed,
                      out long vMemTotal, out long vMemUsed);
        float ramUsageGB = memUsed / 1073741824f;
        Debug.Log($"RAM: {ramUsageGB:F2} GB");
    }
}
```

## ⚡ Performance Notes

- **PDH Queries**: The library uses Windows Performance Data Helper (PDH) for most counters
- **Sampling Rate**: Call `Initialize()` once at startup, then query functions at your desired interval (recommended: 500ms - 2s)
- **Thread Safety**: Most functions are thread-safe as they use isolated PDH queries
- **Overhead**: Minimal performance impact; PDH is the same system Task Manager uses

## 🌡️ Temperature Monitoring

Temperature monitoring functions (`GetCPUTemperature()`, `GetGPUTemperature()`, `GetMotherboardTemperature()`, `GetFanSpeed()`) are currently **placeholders** returning -1.

To implement these, you would need:

### 🔥 For CPU/Motherboard Temperatures:
1. **WMI Queries** (Windows Management Instrumentation)
   - Namespace: `root\WMI` or `root\OpenHardwareMonitor`
   - Requires COM initialization and complex queries
   
2. **Hardware Monitoring Libraries**
   - LibreHardwareMonitor (open source)
   - OpenHardwareMonitor
   
3. **MSR Reads** (Model-Specific Registers)
   - Requires kernel driver
   - Hardware-specific (Intel vs AMD)

### 🎮 For GPU Temperatures:
1. **NVIDIA GPUs**: NVIDIA NVAPI SDK
2. **AMD GPUs**: AMD Display Library (ADL) SDK
3. **Intel GPUs**: Intel Graphics API

## ⚠️ Limitations

- **GPU Counter Availability**: GPU utilization counters may not be available on older Windows versions or certain GPU drivers
- **Network Interface Wildcards**: Network counters use wildcards and may aggregate all interfaces
- **Administrative Privileges**: Some advanced features may require elevation
- **Temperature APIs**: Not included due to hardware-specific requirements

## 🔨 Building

### 🏗️ Visual Studio
1. Open the solution in Visual Studio 2017 or later
2. Ensure C++17 standard is enabled
3. Build the project (Release x64 recommended)

### 📦 Compiler Requirements
- C++17 or later
- Windows SDK 10.0 or later

## 📄 License

This is a utility library. Check your project's license requirements.

## 🤝 Contributing

To extend functionality:
1. Add new PDH counters in `Initialize()`
2. Create getter functions in the `extern "C"` block
3. Document in `SystemMonitorAPI.h`
4. Update this README

## 📌 Version History

- **v1.0** - Initial comprehensive release
  - CPU, Memory, Disk, Network, GPU monitoring
  - Battery status and system information
  - Mouse hooks and utility functions

## 🆘 Support

For hardware-specific temperature monitoring implementation, consider:
- **LibreHardwareMonitor**: https://github.com/LibreHardwareMonitor/LibreHardwareMonitor
- **NVIDIA NVAPI**: https://developer.nvidia.com/nvapi
- **AMD ADL**: https://github.com/GPUOpen-LibrariesAndSDKs/display-library

---

**Note**: This library focuses on software-accessible metrics via Windows APIs. For deep hardware monitoring (temperatures, voltages, fan speeds), integration with hardware monitoring libraries is required.
