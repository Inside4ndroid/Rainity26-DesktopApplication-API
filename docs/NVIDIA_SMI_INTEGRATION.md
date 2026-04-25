# NVIDIA-SMI Integration for Power Monitoring

## Overview

The GPU monitoring system now supports reading **GPU power draw** via `nvidia-smi` (NVIDIA System Management Interface), which is included with NVIDIA drivers.

## What's Now Available

### With NVAPI (Built-in):
- GPU Temperature
- Core Clock Speed
- Memory Clock Speed
- GPU Usage %
- Fan Speed (RPM & %)

### With nvidia-smi (New):
- **GPU Power Draw** (Watts)
- **GPU Power Limit** (Max Watts)

### Still Unavailable:
- GPU Voltage (requires hardware sensors or MSI Afterburner SDK)

---

## How It Works

### Automatic Detection
The system automatically:
1. Detects if `nvidia-smi.exe` is available on the system
2. Queries power data via command-line interface
3. Caches results for 1 second to minimize overhead
4. Gracefully falls back if nvidia-smi is not found

### nvidia-smi Query Format
```bash
nvidia-smi --query-gpu=power.draw,power.limit --format=csv,noheader,nounits -i 0
```

**Output Example:**
```
145.32, 450.00
```
- `145.32` = Current power draw in Watts
- `450.00` = Power limit (TDP) in Watts

---

## Expected Results

### RTX 4090 Example:
```
[GetGPUInformation] Name: 'NVIDIA GeForce RTX 4090'
[GetGPUInformation] Vendor: NVIDIA
[GetGPUInformation] Usage: 45.2%
[GetGPUInformation] Temp: 62�C
[GetGPUInformation] Core clock: 2520 MHz
[GetGPUInformation] Memory clock: 10501 MHz
[GetGPUInformation] Fan: 1850 RPM (61%)
[GetGPUInformation] Power draw: 325.5 W     # NEW! via nvidia-smi
[GetGPUInformation] Voltage: unavailable
```

### RTX 3080 Example:
```
[GetGPUInformation] Power draw: 285.0 W
[GetGPUInformation] Power limit: 350.0 W
```

---

## Performance & Caching

### Caching Strategy:
- **Cache Lifetime**: 1 second
- **Reason**: nvidia-smi takes ~50-100ms to execute
- **Update Frequency**: Data refreshes automatically every 1 second

### Performance Impact:
- **First Call**: ~50-100ms (executes nvidia-smi)
- **Subsequent Calls (within 1s)**: <1ms (uses cache)
- **CPU Usage**: Minimal (<0.1%)
- **Memory**: ~4KB for output buffer

### Optimization:
The system only queries power data when `GetGPUMetrics()` or `GetGPUPowerDraw()` is called, so there's no background polling.

---

## Troubleshooting

### Power draw returns -1.0 (unavailable)

**Possible Causes:**
1. **nvidia-smi not found** - NVIDIA drivers not installed
2. **nvidia-smi not in PATH** - Can't locate executable
3. **Insufficient permissions** - Process can't execute nvidia-smi
4. **GPU not supported** - Very old GPUs may not report power

**Solutions:**

#### 1. Check if nvidia-smi is installed:
```cmd
nvidia-smi
```

If this works in Command Prompt, it should work in the DLL.

#### 2. Manually add nvidia-smi to PATH:
```cmd
# Default location (64-bit):
C:\Program Files\NVIDIA Corporation\NVSMI\nvidia-smi.exe

# Add to system PATH or copy to C:\Windows\System32\
```

#### 3. Verify GPU supports power reporting:
```cmd
nvidia-smi --query-gpu=power.draw --format=csv
```

If this returns "N/A", your GPU doesn't support power monitoring.

---

## Advanced: Available nvidia-smi Queries

### Full Power Stats:
```bash
nvidia-smi --query-gpu=power.draw,power.limit,power.default_limit,power.min_limit,power.max_limit --format=csv
```

### All Sensors:
```bash
nvidia-smi --query-gpu=timestamp,name,temperature.gpu,utilization.gpu,utilization.memory,memory.total,memory.used,memory.free,power.draw,clocks.current.graphics,clocks.current.sm,clocks.current.memory,clocks.current.video --format=csv
```

### Voltage (Limited Support):
Voltage is **not directly available** via nvidia-smi on most consumer GPUs. It requires:
- Hardware monitoring tools (HWiNFO, MSI Afterburner)
- Direct sensor access (requires kernel drivers)
- NVIDIA NVML library (enterprise GPUs only)

---

## Future Enhancements

### Possible Additions:
1. **GPU Memory Usage** (via nvidia-smi)
2. **GPU Memory Clock** (via nvidia-smi)
3. **PCIe Bandwidth** (via nvidia-smi)
4. **ECC Error Counts** (via nvidia-smi, Quadro/Tesla only)
5. **Voltage** (requires MSI Afterburner SDK or HWiNFO integration)

### Implementation Ideas:

#### Add Memory Usage:
```cpp
// In ExecuteNvidiaSmi query:
"--query-gpu=power.draw,power.limit,memory.used,memory.total"

// Parse additional fields:
metrics.memoryUsed = std::stoull(token) * 1024 * 1024; // Convert MB to bytes
```

#### Add PCIe Stats:
```cpp
"--query-gpu=pcie.link.gen.current,pcie.link.width.current"
```

---

## C# Integration

### Get Power Draw:
```csharp
[DllImport("DesktopApplication.dll")]
static extern float GetGPUPowerDraw();

// Usage:
float power = GetGPUPowerDraw();
if (power >= 0)
    Console.WriteLine($"GPU Power: {power:F1} W");
else
    Console.WriteLine("Power monitoring unavailable");
```

### Get Full Metrics:
```csharp
// Already works with existing functions!
float power = GetGPUPowerDraw();
float temp = GetGPUTemperature();
int clock = GetGPUClockSpeed();
int fan = GetGPUFanSpeed();

Console.WriteLine($"Power: {power}W | Temp: {temp}�C | Clock: {clock} MHz | Fan: {fan} RPM");
```

---

## Configuration

### Adjust Cache Lifetime:
In `GPUMonitor.cpp`, find:
```cpp
const DWORD cacheLifetime = 1000; // Cache for 1 second
```

Change to:
```cpp
const DWORD cacheLifetime = 500;  // 0.5 seconds (more responsive)
const DWORD cacheLifetime = 2000; // 2 seconds (less overhead)
```

### Disable nvidia-smi Integration:
Comment out the power query in `GetGPUMetrics()`:
```cpp
// Internal::UpdateNvidiaSmiData();
// metrics.powerAvailable = false;
```

---

## Comparison: NVAPI vs nvidia-smi

| Metric | NVAPI | nvidia-smi | Winner |
|--------|-------|------------|--------|
| Temperature | Yes (fast) | Yes | NVAPI (faster) |
| Clock Speed | Yes (fast) | Yes | NVAPI (faster) |
| Fan Speed | Yes (fast) | Yes | NVAPI (faster) |
| GPU Usage | Yes (fast) | Yes | NVAPI (faster) |
| **Power Draw** | No | Yes | **nvidia-smi** |
| Voltage | No | No | Neither |
| Latency | <1ms | ~50ms | NVAPI |
| Admin Required | No | No | Tie |

**Recommendation**: Use NVAPI for everything except power draw, which requires nvidia-smi.

---

## Summary

### What Changed:
1. Added nvidia-smi command execution
2. Added power data parsing (CSV format)
3. Added 1-second caching to minimize overhead
4. Integrated into existing GetGPUMetrics() API
5. Graceful fallback if nvidia-smi unavailable

### What's New:
- **GPU Power Draw** (Watts) - Real-time power consumption
- **GPU Power Limit** (Watts) - Maximum TDP

### Still TODO:
- GPU Voltage (requires MSI Afterburner SDK or HWiNFO)
- Per-rail power breakdown (requires enterprise NVML)
- Power efficiency metrics (performance per watt)

---

**Power monitoring now works out of the box if you have NVIDIA drivers installed!**

Test it with your Unity application and you should now see real power draw values instead of "unavailable".
