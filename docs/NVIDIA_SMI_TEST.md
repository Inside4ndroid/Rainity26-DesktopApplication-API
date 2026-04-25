# Quick Test: nvidia-smi Power Monitoring

## Verify nvidia-smi Works

### 1. Open Command Prompt (Win+R, type `cmd`)

### 2. Test nvidia-smi:
```cmd
nvidia-smi
```

**Expected Output:**
```
+-----------------------------------------------------------------------------------------+
| NVIDIA-SMI 545.84       Driver Version: 545.84       CUDA Version: 12.3     |
|-------------------------------+------------------------+----------------------+
| GPU  Name            TCC/WDDM | Bus-Id        Disp.A | Volatile Uncorr. ECC |
| Fan  Temp   Perf  Pwr:Usage/Cap |           Memory-Usage | GPU-Util  Compute M. |
|===============================+========================+======================|
|   0  NVIDIA GeForce ... WDDM  |   00000000:01:00.0  On |                  N/A |
| 30%   35C    P8    25W /  450W |    1024MiB / 24564MiB |      2%      Default |
+-----------------------------------------------------------------------------------------+
```

### 3. Test Power Query:
```cmd
nvidia-smi --query-gpu=power.draw,power.limit --format=csv,noheader,nounits
```

**Expected Output:**
```
145.32, 450.00
```

If you see numbers, it works!

---

## Test in Your Application

### C# Test Code:
```csharp
using UnityEngine;
using System.Runtime.InteropServices;

public class GPUPowerTest : MonoBehaviour
{
    [DllImport("DesktopApplication")]
    private static extern bool Initialize();
    
    [DllImport("DesktopApplication")]
    private static extern float GetGPUPowerDraw();
    
    [DllImport("DesktopApplication")]
    private static extern float GetGPUTemperature();
    
    [DllImport("DesktopApplication")]
    private static extern int GetGPUClockSpeed();
    
    [DllImport("DesktopApplication")]
    private static extern int GetGPUFanSpeed();

    void Start()
    {
        if (!Initialize())
        {
            Debug.LogError("Failed to initialize GPU monitoring!");
            return;
        }
        
        InvokeRepeating(nameof(UpdateGPUStats), 1f, 1f);
    }

    void UpdateGPUStats()
    {
        float power = GetGPUPowerDraw();
        float temp = GetGPUTemperature();
        int clock = GetGPUClockSpeed();
        int fan = GetGPUFanSpeed();
        
        Debug.Log($"=== GPU Stats ===");
        Debug.Log($"Power: {(power >= 0 ? $"{power:F1} W" : "unavailable")}");
        Debug.Log($"Temp: {temp:F1}�C");
        Debug.Log($"Clock: {clock} MHz");
        Debug.Log($"Fan: {fan} RPM");
        Debug.Log($"================");
    }
}
```

### Expected Console Output:
```
=== GPU Stats ===
Power: 145.3 W          # Should now show real value!
Temp: 62.0�C
Clock: 2520 MHz
Fan: 1850 RPM
================
```

---

## Troubleshooting

### If Power Still Shows "unavailable":

#### 1. Check nvidia-smi in Command Prompt
```cmd
nvidia-smi --query-gpu=power.draw --format=csv
```

If this shows "N/A", your GPU doesn't support power monitoring.

#### 2. Check PATH Environment Variable
```cmd
where nvidia-smi
```

Should show:
```
C:\Program Files\NVIDIA Corporation\NVSMI\nvidia-smi.exe
```

If not found, add to PATH:
```cmd
setx PATH "%PATH%;C:\Program Files\NVIDIA Corporation\NVSMI"
```

#### 3. Manually Test nvidia-smi Path
Edit `GPUMonitor.cpp` temporarily:
```cpp
// Change this line:
std::string cmdLine = "nvidia-smi " + arguments;

// To this (use full path):
std::string cmdLine = "\"C:\\Program Files\\NVIDIA Corporation\\NVSMI\\nvidia-smi.exe\" " + arguments;
```

#### 4. Check Process Creation
Add debug logging to `ExecuteNvidiaSmi`:
```cpp
if (!CreateProcessA(...))
{
    DWORD error = GetLastError();
    // Log error code (2 = file not found, 5 = access denied)
    CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);
    return "";
}
```

---

## Performance Test

### Measure nvidia-smi Latency:
```cmd
# Run 10 times and measure average time
for /l %i in (1,1,10) do @echo | time & nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits & echo | time
```

**Typical Results:**
- First call: 80-120ms (DLL loading)
- Subsequent calls: 40-60ms (cached)

### With 1-Second Cache (Current Implementation):
- Calls within 1s: <1ms (cache hit)
- Calls after 1s: ~50ms (cache miss, refresh)

---

## Validation Checklist

- [ ] nvidia-smi runs in Command Prompt
- [ ] Power query returns numbers (not "N/A")
- [ ] DLL builds successfully
- [ ] Unity application initializes GPU monitoring
- [ ] GetGPUPowerDraw() returns positive value (not -1)
- [ ] Power value updates over time (changes with GPU load)
- [ ] Cache works (second call within 1s is instant)

---

## Advanced: Test Different GPU Loads

### 1. Idle State:
Run your Unity app with minimal scene.
Expected: 10-50W (RTX 4090), 5-20W (RTX 3060)

### 2. Light Load:
Run a simple 3D scene.
Expected: 50-150W

### 3. Heavy Load:
Run GPU-intensive shader or particle effects.
Expected: 200-400W (depending on TDP)

### Monitor Changes:
```csharp
void Update()
{
    if (Input.GetKeyDown(KeyCode.P))
    {
        float power = GetGPUPowerDraw();
        Debug.Log($"Current Power: {power:F1} W");
    }
}
```

Press 'P' to check power at any time!

---

## Success Indicators

### Power Monitoring is Working If:
1. Power value is between 10W and your GPU's TDP (e.g., 450W for RTX 4090)
2. Power changes when GPU load changes
3. Power increases when rendering heavy scenes
4. Power decreases when minimizing/idle
5. First call takes ~50ms, subsequent calls <1ms

### Example Valid Values:
```
RTX 4090:   15W (idle) to 450W (max)
RTX 4080:   10W (idle) to 320W (max)
RTX 3090:   20W (idle) to 350W (max)
RTX 3080:   15W (idle) to 320W (max)
RTX 3060:   10W (idle) to 170W (max)
```

If your values are in this range and change with load, **it's working perfectly!**

---

**Ready to test?** Rebuild your DLL, copy it to Unity, and run your application! Power monitoring should now be fully functional.
