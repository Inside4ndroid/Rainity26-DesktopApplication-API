# System Monitor API - Quick Reference Guide

## ?? Quick Start

### Minimal Example
```cpp
// 1. Initialize
Initialize();

// 2. Get CPU usage
float cpuUsage = GetCPUPercentPDH();

// 3. Get Memory info
long long memTotal, memUsed, vMemTotal, vMemUsed;
GetMemoryInfo(&memTotal, &memUsed, &vMemTotal, &vMemUsed);
```

### C# Example
```csharp
[DllImport("DesktopApplication.dll")]
private static extern bool Initialize();

[DllImport("DesktopApplication.dll")]
private static extern float GetCPUPercentPDH();

// Usage
Initialize();
float cpu = GetCPUPercentPDH();
Console.WriteLine($"CPU: {cpu}%");
```

## ?? Function Categories

### Initialization
| Function | Description | Returns |
|----------|-------------|---------|
| `Initialize()` | Must call first | `bool` |

### CPU Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetCPUPercentPDH()` | Overall CPU usage | `float` (0-100) |
| `GetCPUCoreCount()` | Number of CPU cores | `int` |
| `GetCPUCorePercent(int)` | Usage of specific core | `float` (0-100) |
| `GetCPUFrequency()` | Base frequency in MHz | `int` |
| `GetCPUName(wchar_t*, int)` | CPU brand string | `bool` |
| `GetCPUTemperature()` | CPU temp in Celsius | `float` (-1 if unavailable) |

### Memory Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetMemoryInfo(...)` | RAM and VRAM usage | `bool` |
| `GetPageFileUsage(...)` | Virtual memory usage | `bool` |

### Disk Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetDiskIOStats(...)` | Read/write bytes/sec | `bool` |
| `GetDiskSpace(wchar_t, ...)` | Total/free space | `bool` |

### Network Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetNetworkStats(...)` | Upload/download bytes/sec | `bool` |

### GPU Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetGPUUsagePercent()` | GPU utilization | `float` (0-100) |
| `GetGPUName(wchar_t*, int)` | GPU name | `bool` |
| `GetGPUTemperature()` | GPU temp | `float` (-1 if unavailable) |
| `GetGPUClockSpeed()` | GPU clock speed MHz | `int` |

### System Info Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetSystemUptime()` | Seconds since boot | `long long` |
| `GetProcessCount()` | Running processes | `int` |
| `GetThreadCount()` | Total threads | `int` |
| `GetHandleCount()` | Total handles | `int` |
| `GetSystemInfoString(...)` | Windows version | `bool` |

### Battery Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetBatteryStatus(...)` | Battery info | `bool` |

### Utility Functions
| Function | Description | Returns |
|----------|-------------|---------|
| `GetJumboIcon(...)` | 256x256 icon pixels | `BYTE*` |
| `SimulateKeypress(uint)` | Send virtual key | `bool` |
| `GetAvatarPath(...)` | User avatar path | `bool` |
| `SetMouseHook(HINSTANCE)` | Install mouse hook | `bool` |
| `UnhookMouseHook()` | Remove mouse hook | `bool` |
| `GetMouseDown()` | Check mouse down event | `bool` |
| `GetMouseUp()` | Check mouse up event | `bool` |

## ?? Common Patterns

### Monitor Everything (C++)
```cpp
Initialize();

// CPU
float cpu = GetCPUPercentPDH();
int cores = GetCPUCoreCount();

// Memory
long long mt, mu, vmt, vmu;
GetMemoryInfo(&mt, &mu, &vmt, &vmu);

// Disk
long long dr, dw;
GetDiskIOStats(&dr, &dw);

// Network
long long ns, nr;
GetNetworkStats(&ns, &nr);

// GPU
float gpu = GetGPUUsagePercent();
```

### Continuous Monitoring (C#)
```csharp
Initialize();

while (true)
{
    float cpu = GetCPUPercentPDH();
    GetMemoryInfo(out long mt, out long mu, out long vmt, out long vmu);
    
    Console.WriteLine($"CPU: {cpu:F1}%  RAM: {mu/1024/1024/1024:F2}GB");
    
    Thread.Sleep(1000); // Update every second
}
```

### Unity Integration
```csharp
void Start()
{
    Initialize();
}

void Update()
{
    if (Time.frameCount % 30 == 0) // Every 30 frames
    {
        float cpu = GetCPUPercentPDH();
        debugText.text = $"CPU: {cpu:F1}%";
    }
}
```

## ?? Important Notes

### Initialization
- **Must call `Initialize()` before any other function**
- Call once at application startup
- Returns `false` if PDH initialization fails

### Sampling Frequency
- **Recommended**: 500ms - 2 seconds between queries
- Too frequent polling may affect performance
- PDH counters need time between samples

### Return Values
- **Temperature functions**: Return `-1.0f` if not available
- **Boolean functions**: Return `false` on error
- **Percentage functions**: Return `0.0f` on error

### Platform Requirements
- **Windows 8.1+** for DXGI 1.4 features (VRAM usage)
- **Windows 10+** recommended for all features
- Some GPU counters may not work on older systems

### Thread Safety
- Most functions are thread-safe
- Can be called from different threads
- PDH queries are internally synchronized

## ?? Use Cases

### Performance Monitoring Dashboard
```csharp
// Create a dashboard that updates every second
var timer = new Timer(1000);
timer.Elapsed += (s, e) =>
{
    UpdateDashboard();
};
```

### Game Performance Overlay
```csharp
// Show FPS + system stats in game
void OnGUI()
{
    float cpu = GetCPUPercentPDH();
    float gpu = GetGPUUsagePercent();
    GUI.Label(new Rect(10, 10, 200, 20), 
              $"CPU: {cpu:F1}%  GPU: {gpu:F1}%");
}
```

### System Health Alerts
```csharp
// Alert when resources are high
if (GetCPUPercentPDH() > 90)
    ShowAlert("High CPU Usage!");
    
GetMemoryInfo(out _, out long mu, out long mt, out _);
if ((mu * 100 / mt) > 90)
    ShowAlert("Low Memory!");
```

### Historical Tracking
```csharp
// Track metrics over time
var history = new List<float>();
history.Add(GetCPUPercentPDH());

float average = history.Average();
float max = history.Max();
```

## ?? Troubleshooting

### "Initialize() returns false"
- Check if PDH service is running
- Run as administrator (some counters require elevation)
- Verify Windows version compatibility

### "GPU counters return 0"
- Update GPU drivers
- Some GPUs don't expose these counters
- Try `GetMemoryInfo()` for VRAM as alternative

### "Temperature functions return -1"
- Not implemented yet - requires hardware libraries
- Use third-party tools (HWiNFO, OpenHardwareMonitor)
- Implement using WMI or vendor SDKs

### "High CPU usage from monitoring"
- Increase polling interval (>= 1 second)
- Don't query per-core usage unless needed
- Disable GPU counters if not required

## ?? Files Included

- `dllmain.cpp` - Main DLL implementation
- `SystemMonitorAPI.h` - C/C++ header file
- `README.md` - Complete documentation
- `Examples/SystemMonitorExample.cs` - C# console example
- `Examples/SystemMonitorUnity.cs` - Unity MonoBehaviour
- `Examples/AdvancedSystemMonitor.cs` - Historical tracking example

## ?? Next Steps

1. **Basic Usage**: Start with `SystemMonitorExample.cs`
2. **Unity Integration**: Use `SystemMonitorUnity.cs` as template
3. **Advanced Features**: Explore `AdvancedSystemMonitor.cs` for graphs/stats
4. **Custom Implementation**: Modify `dllmain.cpp` to add new metrics

## ?? License

Check your project license requirements.

## ?? Tips

- Cache DLL import declarations in a static class
- Use interpolation for smooth UI updates
- Batch multiple queries in one update cycle
- Consider background thread for monitoring
- Log data for performance analysis
