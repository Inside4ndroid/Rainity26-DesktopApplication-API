# GPU Monitoring API - Quick Reference

## ?? New GPU Functions (7 Total)

| Function | Return Type | Description | Notes |
|----------|-------------|-------------|-------|
| `GetGPUTemperature()` | `float` | GPU temperature in °C | -1.0 if unavailable |
| `GetGPUClockSpeed()` | `int` | Core clock in MHz | 0 if unavailable |
| `GetGPUMemoryClock()` | `int` | Memory clock in MHz | 0 if unavailable |
| `GetGPUFanSpeed()` | `int` | Fan speed in RPM | -1 if unavailable |
| `GetGPUFanSpeedPercent()` | `int` | Fan speed 0-100% | -1 if unavailable |
| `GetGPUPowerDraw()` | `float` | Power consumption in Watts | -1.0 if unavailable |
| `GetGPUVoltage()` | `float` | Core voltage in Volts | -1.0 if unavailable |
| `GetGPUVendorID()` | `uint` | Vendor ID | 10DE, 1002, 8086, or 0 |

## ?? Quick Start

### Without SDKs (Works Now):
```csharp
Initialize();
uint vendor = GetGPUVendorID();  // Works!
// Temperature/clocks return -1/0 (unavailable)
```

### With NVIDIA SDK:
1. Download NVAPI from https://developer.nvidia.com/nvapi
2. Copy `nvapi.h`, `nvapi64.lib` to `DesktopApplication\`
3. Edit `GPUMonitor.cpp`, uncomment: `#define ENABLE_NVIDIA_NVAPI`
4. Build ? Full monitoring enabled!

### With AMD SDK:
1. Download ADL from https://github.com/GPUOpen-LibrariesAndSDKs/display-library
2. Copy ADL headers to `DesktopApplication\`
3. Edit `GPUMonitor.cpp`, uncomment: `#define ENABLE_AMD_ADL`
4. Build ? Full monitoring enabled!

## ?? C# Example

```csharp
using System;
using System.Runtime.InteropServices;

class GPUTest
{
    [DllImport("DesktopApplication.dll")]
    static extern bool Initialize();
    
    [DllImport("DesktopApplication.dll")]
    static extern uint GetGPUVendorID();
    
    [DllImport("DesktopApplication.dll")]
    static extern float GetGPUTemperature();
    
    [DllImport("DesktopApplication.dll")]
    static extern int GetGPUClockSpeed();
    
    [DllImport("DesktopApplication.dll")]
    static extern int GetGPUMemoryClock();
    
    [DllImport("DesktopApplication.dll")]
    static extern int GetGPUFanSpeed();
    
    [DllImport("DesktopApplication.dll")]
    static extern int GetGPUFanSpeedPercent();
    
    [DllImport("DesktopApplication.dll")]
    static extern float GetGPUPowerDraw();
    
    [DllImport("DesktopApplication.dll")]
    static extern float GetGPUVoltage();
    
    static void Main()
    {
        if (!Initialize())
        {
            Console.WriteLine("Failed to initialize!");
            return;
        }
        
        // Detect GPU vendor
        uint vendorId = GetGPUVendorID();
        string vendorName = vendorId switch
        {
            0x10DE => "NVIDIA",
            0x1002 => "AMD",
            0x8086 => "Intel",
            _ => "Unknown"
        };
        
        Console.WriteLine($"GPU Vendor: {vendorName} (0x{vendorId:X4})");
        
        // Get temperature
        float temp = GetGPUTemperature();
        if (temp >= 0)
            Console.WriteLine($"Temperature: {temp:F1}°C");
        else
            Console.WriteLine("Temperature: Not available (enable SDK)");
        
        // Get clock speeds
        int coreClock = GetGPUClockSpeed();
        int memClock = GetGPUMemoryClock();
        if (coreClock > 0)
            Console.WriteLine($"GPU Clock: {coreClock} MHz");
        if (memClock > 0)
            Console.WriteLine($"Memory Clock: {memClock} MHz");
        
        // Get fan info
        int fanRPM = GetGPUFanSpeed();
        int fanPercent = GetGPUFanSpeedPercent();
        if (fanRPM >= 0)
            Console.WriteLine($"Fan Speed: {fanRPM} RPM ({fanPercent}%)");
        
        // Get power & voltage
        float power = GetGPUPowerDraw();
        float voltage = GetGPUVoltage();
        if (power >= 0)
            Console.WriteLine($"Power Draw: {power:F1} W");
        if (voltage >= 0)
            Console.WriteLine($"Core Voltage: {voltage:F3} V");
    }
}
```

## ?? Vendor Support Matrix

| Feature | NVIDIA (NVAPI) | AMD (ADL) | Intel |
|---------|---------------|-----------|-------|
| Temperature | ? Full | ? Full | ? |
| Core Clock | ? | ? | ? |
| Memory Clock | ? | ? | ? |
| Fan Speed (RPM) | ? | ? | N/A |
| Fan Speed (%) | ? | ? | N/A |
| Power Draw | ? | ?? Limited | ? |
| Voltage | ? | ?? Limited | ? |
| Vendor ID | ? | ? | ? |

? = Fully supported  
?? = Partially supported  
? = Not available  
N/A = Not applicable  

## ?? Setup Required?

### Option 1: Use Without SDKs (Works Now!)
- ? Vendor detection works
- ? GPU name works
- ? VRAM usage works
- ? Temperature/clocks unavailable

### Option 2: Add NVIDIA Support (5 minutes)
1. Register at nvidia.com/developer
2. Download NVAPI
3. Copy 2 files
4. Uncomment 1 line
5. Build ? Done!

### Option 3: Add AMD Support (10 minutes)
1. Download from GitHub
2. Copy 3 files
3. Uncomment 1 line
4. Implement ADL init (code provided)
5. Build ? Done!

## ? Performance

- **Overhead**: Minimal (<1% CPU)
- **Update Rate**: Poll every 1-2 seconds
- **Thread Safe**: Yes
- **Admin Required**: No (for most features)

## ?? Full Documentation

See `GPU_SETUP_GUIDE.md` for complete setup instructions!

---

**Ready to use NOW!** Vendor detection and basic GPU info work immediately. Enable SDKs for temperature/clock monitoring. ??
