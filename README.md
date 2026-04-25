# Rainity26 Desktop Application API

A comprehensive **C++ DLL library** for Windows system monitoring with advanced GPU vendor API support. Monitor CPU, memory, disk, network, GPU metrics, and more through a simple, high-performance native API.

[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

---

## ?? Features

### ?? CPU Monitoring
- **Overall CPU usage** with frequency adjustment (matches Task Manager)
- **Per-core/thread monitoring** for individual CPU cores
- **CPU information**: Brand name, core count, base frequency
- **Temperature monitoring**: Framework ready (requires WMI/hardware libraries)

### ?? Memory Monitoring
- **Physical RAM**: Total and used system memory
- **GPU VRAM**: Video memory usage via DXGI (Windows 8.1+)
- **Page File**: Virtual memory statistics

### ?? Disk Monitoring
- **Disk I/O**: Real-time read/write throughput (bytes/sec)
- **Disk Space**: Total and free space per drive letter

### ?? Network Monitoring
- **Network throughput**: Bytes sent/received per second across all interfaces

### ?? **GPU Monitoring** ? (Advanced)
- **Multi-vendor support**: NVIDIA (NVAPI), AMD (ADL), Intel
- **GPU temperature** (via vendor APIs)
- **Clock speeds**: Core and memory clocks
- **GPU usage %** (via dynamic P-states)
- **Fan speed**: RPM and percentage
- **Power draw**: Via nvidia-smi integration
- **VRAM usage**: Dedicated video memory tracking
- **Automatic vendor detection**

### ?? Battery & Power
- **Battery status**: Charge percentage, charging state
- **Remaining time**: Estimated battery life
- **Power info**: AC line status

### ?? System Information
- **System uptime**: Time since last boot
- **Process statistics**: Process, thread, and handle counts
- **Windows version**: OS version and build number

### ??? Input & UI Utilities
- **Mouse hooks**: Low-level mouse event monitoring
- **Keyboard simulation**: Send virtual key presses
- **Icon rendering**: Extract 256×256 shell icons
- **User avatar**: Retrieve Windows account picture path

---

## ?? Requirements

- **Windows 8.1 or later** (Windows 10/11 recommended)
- **C++17 compiler** (Visual Studio 2017+)
- **Administrator privileges** (optional, for some advanced features)

### Build Dependencies
- `pdh.lib` - Performance Data Helper
- `dxgi.lib` - DirectX Graphics Infrastructure
- `powrprof.lib` - Power management
- `setupapi.lib` - Device setup
- `iphlpapi.lib` - IP Helper
- `psapi.lib` - Process status
- Standard Windows libraries: `shell32`, `comctl32`, `user32`, `gdi32`

---

## ??? Building the Project

### Visual Studio

1. **Clone the repository:**
   ```bash
   git clone https://github.com/Inside4ndroid/Rainity26-DesktopApplication-API.git
   cd Rainity26-DesktopApplication-API
   ```

2. **Open the solution:**
   - Open `DesktopApplication.sln` in Visual Studio 2017 or later

3. **Configure build settings:**
   - Ensure C++17 standard is enabled
   - Select **Release x64** configuration (recommended)

4. **Build:**
   - Build ? Build Solution (Ctrl+Shift+B)
   - Output: `bin/Release/DesktopApplication.dll`

### Command Line

```bash
msbuild DesktopApplication.sln /p:Configuration=Release /p:Platform=x64
```

---

## ?? GPU Vendor API Setup (Optional)

The DLL works **out-of-the-box** for basic GPU monitoring. For advanced features (temperature, clocks, fan speed), enable vendor SDKs:

### **NVIDIA GPUs** (Recommended)

1. Download **NVAPI SDK** from [NVIDIA Developer](https://developer.nvidia.com/nvapi)
2. Copy ALL header files to `DesktopApplication/`
3. Ensure `nvapi64.lib` is in `DesktopApplication/`
4. Already enabled in the codebase ?

### **AMD GPUs**

1. Download **ADL SDK** from [GitHub](https://github.com/GPUOpen-LibrariesAndSDKs/display-library)
2. Copy ADL headers to `DesktopApplication/`
3. Uncomment `#define ENABLE_AMD_ADL` in `GPUMonitor.cpp`
4. Rebuild

### **Intel GPUs**

- Basic support works out-of-the-box (no SDK required)
- Limited to: GPU name, VRAM, basic usage

For detailed setup instructions, see: [**docs/GPU_SETUP_GUIDE.md**](docs/GPU_SETUP_GUIDE.md)

---

## ?? Quick Start

### C++ Example

```cpp
#include <windows.h>
#include <stdio.h>

// Function declarations (import from DLL)
extern "C" {
    __declspec(dllimport) bool Initialize();
    __declspec(dllimport) float GetCPUPercentPDH();
    __declspec(dllimport) bool GetMemoryInfo(long long*, long long*, long long*, long long*);
    __declspec(dllimport) float GetGPUTemperature();
    __declspec(dllimport) int GetGPUClockSpeed();
}

int main() {
    // Initialize the library
    if (!Initialize()) {
        printf("Failed to initialize!\n");
        return 1;
    }

    // Get CPU usage
    float cpuUsage = GetCPUPercentPDH();
    printf("CPU Usage: %.2f%%\n", cpuUsage);

    // Get memory info
    long long memTotal, memUsed, vMemTotal, vMemUsed;
    if (GetMemoryInfo(&memTotal, &memUsed, &vMemTotal, &vMemUsed)) {
        printf("RAM: %.2f GB / %.2f GB\n", 
               memUsed / 1073741824.0, 
               memTotal / 1073741824.0);
    }

    // Get GPU info
    float gpuTemp = GetGPUTemperature();
    int gpuClock = GetGPUClockSpeed();
    printf("GPU: %.0f°C @ %d MHz\n", gpuTemp, gpuClock);

    return 0;
}
```

### C# / Unity Example

```csharp
using System;
using System.Runtime.InteropServices;
using UnityEngine;

public class SystemMonitor : MonoBehaviour
{
    const string DLL = "DesktopApplication";

    [DllImport(DLL)] static extern bool Initialize();
    [DllImport(DLL)] static extern float GetCPUPercentPDH();
    [DllImport(DLL)] static extern float GetGPUTemperature();
    [DllImport(DLL)] static extern int GetGPUClockSpeed();
    [DllImport(DLL)] static extern float GetGPUUsagePercent();

    void Start()
    {
        if (!Initialize())
        {
            Debug.LogError("Failed to initialize system monitor!");
            return;
        }
    }

    void Update()
    {
        float cpuUsage = GetCPUPercentPDH();
        float gpuUsage = GetGPUUsagePercent();
        float gpuTemp = GetGPUTemperature();
        int gpuClock = GetGPUClockSpeed();

        Debug.Log($"CPU: {cpuUsage:F1}% | GPU: {gpuUsage:F1}% @ {gpuTemp:F0}°C ({gpuClock} MHz)");
    }
}
```

---

## ?? Documentation

| Document | Description |
|----------|-------------|
| [**API Reference**](docs/README.md) | Complete API documentation with all functions |
| [**GPU Setup Guide**](docs/GPU_SETUP_GUIDE.md) | How to enable NVIDIA/AMD GPU monitoring |
| [**GPU API Reference**](docs/GPU_API_REFERENCE.md) | GPU-specific functions and features |
| [**Quick Reference**](docs/QUICK_REFERENCE.md) | Cheat sheet for common functions |
| [**Feature Matrix**](docs/FEATURE_MATRIX.md) | Platform and GPU vendor support matrix |
| [**NVIDIA-SMI Integration**](docs/NVIDIA_SMI_INTEGRATION.md) | Power monitoring via nvidia-smi |
| [**Implementation Summary**](docs/GPU_MONITORING_SUMMARY.md) | Technical implementation details |

---

## ?? Supported Platforms

| Feature | Windows 8.1+ | Windows 10/11 |
|---------|-------------|---------------|
| CPU Monitoring | ? | ? |
| Memory Monitoring | ? | ? |
| Disk I/O | ? | ? |
| Network Stats | ? | ? |
| GPU Basic (Name, VRAM) | ? | ? |
| GPU NVIDIA (Temp, Clocks) | ? | ? |
| GPU AMD (Temp, Clocks) | ? | ? |
| Power via nvidia-smi | ?? Limited | ? |

---

## ?? GPU Vendor Support Matrix

| Feature | NVIDIA (NVAPI) | AMD (ADL) | Intel |
|---------|---------------|-----------|-------|
| GPU Name | ? | ? | ? |
| VRAM Usage | ? | ? | ? |
| GPU Usage % | ? | ? | ? PDH |
| Temperature | ? | ? | ? |
| Core Clock | ? | ? | ? |
| Memory Clock | ? | ? | ? |
| Fan Speed (RPM) | ? | ? | N/A |
| Fan Speed (%) | ? | ? | N/A |
| Power Draw | ? nvidia-smi | ?? Limited | ? |
| Voltage | ? | ? | ? |

**Legend:**
- ? Fully supported
- ?? Partially supported or requires additional setup
- ? Not available
- N/A Not applicable

---

## ? Performance

- **Overhead**: Minimal (<1% CPU usage)
- **Latency**: <2ms per query (cached data)
- **Thread-safe**: Yes (isolated PDH queries)
- **Memory**: ~1-2 MB runtime footprint
- **nvidia-smi caching**: 1-second cache for power queries

---

## ?? Known Limitations

1. **GPU Power Monitoring (NVIDIA)**:
   - Power draw shows `N/A` on GeForce GPUs in WDDM mode (display mode)
   - This is a **normal NVIDIA driver limitation**
   - See: [NVIDIA Power N/A Explained](docs/NVIDIA_POWER_NA_EXPLAINED.md)

2. **Temperature Monitoring**:
   - CPU/Motherboard temperatures require WMI or hardware libraries
   - GPU temperatures require vendor SDKs (NVAPI for NVIDIA, ADL for AMD)

3. **Administrative Privileges**:
   - Most features work without admin rights
   - Some advanced features may require elevation

4. **AMD GPU Monitoring**:
   - Framework is ready, full implementation requires ADL SDK integration
   - Currently returns placeholder values

---

## ?? Future Enhancements

- [ ] Complete AMD ADL implementation
- [ ] Intel GPU temperature support
- [ ] CPU temperature via WMI
- [ ] MSI Afterburner SDK integration for power/voltage
- [ ] Network interface enumeration
- [ ] Per-process resource usage
- [ ] GPU memory transfer rates
- [ ] PCIe bandwidth monitoring

---

## ?? Contributing

Contributions are welcome! To contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please ensure:
- Code follows C++17 standards
- All new features are documented
- Build passes without warnings
- README and docs are updated

---

## ?? License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## ?? Acknowledgments

- **NVIDIA** for the NVAPI SDK
- **AMD** for the ADL SDK
- **Microsoft** for comprehensive Windows APIs (PDH, DXGI, WMI)
- **OpenHardwareMonitor** project for inspiration

---

## ?? Contact & Support

- **GitHub Issues**: [Report bugs or request features](https://github.com/Inside4ndroid/Rainity26-DesktopApplication-API/issues)
- **Repository**: [Rainity26-DesktopApplication-API](https://github.com/Inside4ndroid/Rainity26-DesktopApplication-API)

---

## ?? Star History

If you find this project useful, please consider giving it a ? star on GitHub!

---

**Made with ?? for the Windows development community**

*Comprehensive system monitoring made simple.*
