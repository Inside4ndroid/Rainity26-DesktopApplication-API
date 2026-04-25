# GPU Vendor API Setup Guide

This guide will help you enable GPU temperature, clock speed, fan speed, power draw, and voltage monitoring for NVIDIA, AMD, and Intel GPUs.

## Current Status

**Framework Ready**: GPU monitoring infrastructure is implemented  
**Vendor SDKs Required**: Need to download and integrate vendor-specific SDKs  
**Fallback Available**: Basic GPU info works without SDKs (name, VRAM, usage via PDH)

---

## NVIDIA GPU Support (NVAPI)

### Step 1: Download NVAPI SDK

1. Go to: **https://developer.nvidia.com/nvapi**
2. Click "Join NVIDIA Developer Program" (free registration)
3. After approval, download **NVAPI R535** or latest version
4. Extract the ZIP file

### Step 2: Copy Required Files

From the extracted NVAPI SDK, copy these files to `DesktopApplication\` folder:

```
Your NVAPI SDK/
nvapi.h          <- copy this
nvapi64.lib     <- copy this (64-bit)
nvapi.lib       <- copy this (32-bit, optional)
NvApiDriverSettings.h  <- copy this
```

**Copy to:**
```
C:\Users\rob_w\Rainity26\NativePlugin~\DesktopApplication\
```

### Step 3: Enable NVAPI in Code

1. Open `DesktopApplication\GPUMonitor.cpp`
2. Find this line (around line 30):
   ```cpp
   // #define ENABLE_NVIDIA_NVAPI
   ```
3. Uncomment it to:
   ```cpp
   #define ENABLE_NVIDIA_NVAPI
   ```

### Step 4: Build Project

Build your solution. It should now link with NVAPI!

### Features Unlocked (NVIDIA):
- GPU Temperature (Core)
- GPU Clock Speed (Core & Memory)
- Fan Speed (RPM & Percentage)
- Power Draw (Watts)
- Voltage (Volts)
- GPU Usage %
- Memory Usage

---

## AMD GPU Support (ADL)

### Step 1: Download ADL SDK

1. Go to: **https://github.com/GPUOpen-LibrariesAndSDKs/display-library**
2. Click "Code" > "Download ZIP"
3. Extract the ZIP file

### Step 2: Copy Required Files

From the ADL SDK, copy these files to `DesktopApplication\` folder:

```
display-library-master/
include/
  +-- adl_sdk.h   <- copy this
  +-- adl_structures.h  <- copy this
  +-- adl_defines.h  <- copy this
```

**Copy to:**
```
C:\Users\rob_w\Rainity26\NativePlugin~\DesktopApplication\
```

### Step 3: Download AMD DLL

ADL requires `atiadlxx.dll` or `atiadlxy.dll` which is included with AMD drivers.

**Option A - Use System DLL:**
- The DLL is already in `C:\Windows\System32\` if you have AMD drivers installed
- No action needed

**Option B - Distribute with App:**
- Copy from `C:\Windows\System32\atiadlxx.dll` to your app folder

### Step 4: Enable ADL in Code

1. Open `DesktopApplication\GPUMonitor.cpp`
2. Find this line (around line 45):
   ```cpp
   // #define ENABLE_AMD_ADL
   ```
3. Uncomment it to:
   ```cpp
   #define ENABLE_AMD_ADL
   ```

### Step 5: Implement ADL Initialization

The ADL initialization requires dynamic loading of `atiadlxx.dll`. I've left this as a placeholder. Here's the complete implementation you'll need:

**In `GPUMonitor.cpp`, replace the `InitializeADL()` function with:**

```cpp
#ifdef ENABLE_AMD_ADL
// ADL function pointers
typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY)();
typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
typedef int (*ADL_OVERDRIVE5_TEMPERATURE_GET)(int, int, ADLTemperature*);
typedef int (*ADL_OVERDRIVE5_FANSPEED_GET)(int, int, ADLFanSpeedValue*);
typedef int (*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int, ADLPMActivity*);

static HMODULE g_adlDll = nullptr;
static ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create = nullptr;
// ... (other function pointers)

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
```

### Features Unlocked (AMD):
- GPU Temperature (Core & Junction)
- GPU Clock Speed (Core & Memory)
- Fan Speed (RPM & Percentage)
- GPU Usage %
- Power Draw (limited support)
- Voltage (limited support)

---

## Intel GPU Support

### Good News!

Intel GPUs have **limited monitoring** but don't require additional SDKs.

### Features Available (Intel):
- GPU Name
- VRAM Total/Used
- GPU Usage (via Windows PDH)
- Temperature (not accessible without WMI)
- Clock Speed (not accessible)
- Fan Speed (not applicable for most iGPUs)

Intel monitoring works out-of-the-box with the current implementation!

---

## File Structure After Setup

```
DesktopApplication\
dllmain.cpp
GPUMonitor.h
GPUMonitor.cpp
nvapi.h            (NVIDIA SDK)
nvapi64.lib       (NVIDIA SDK)
NvApiDriverSettings.h  (NVIDIA SDK)
adl_sdk.h         (AMD SDK)
adl_structures.h  (AMD SDK)
adl_defines.h     (AMD SDK)
```

---

## Building the Project

### Without SDKs (Default):
```
- GPU name, VRAM, basic usage works
- Temperature/clocks return -1 or 0
```

### With NVIDIA SDK:
1. Copy NVAPI files
2. Uncomment `#define ENABLE_NVIDIA_NVAPI`
3. Build -> Full NVIDIA monitoring enabled!

### With AMD SDK:
1. Copy ADL files
2. Uncomment `#define ENABLE_AMD_ADL`
3. Implement ADL initialization (see above)
4. Build -> Full AMD monitoring enabled!

---

## Testing Your Implementation

### C# Test Code:

```csharp
[DllImport("DesktopApplication.dll")]
private static extern bool Initialize();

[DllImport("DesktopApplication.dll")]
private static extern float GetGPUTemperature();

[DllImport("DesktopApplication.dll")]
private static extern int GetGPUClockSpeed();

[DllImport("DesktopApplication.dll")]
private static extern int GetGPUMemoryClock();

[DllImport("DesktopApplication.dll")]
private static extern int GetGPUFanSpeed();

[DllImport("DesktopApplication.dll")]
private static extern float GetGPUPowerDraw();

[DllImport("DesktopApplication.dll")]
private static extern float GetGPUVoltage();

[DllImport("DesktopApplication.dll")]
private static extern uint GetGPUVendorID();

// Usage:
Initialize();

uint vendorId = GetGPUVendorID();
Console.WriteLine($"GPU Vendor: {vendorId:X4}");  // 10DE=NVIDIA, 1002=AMD, 8086=Intel

float temp = GetGPUTemperature();
Console.WriteLine($"GPU Temp: {temp}�C");

int coreClock = GetGPUClockSpeed();
Console.WriteLine($"GPU Clock: {coreClock} MHz");

int memClock = GetGPUMemoryClock();
Console.WriteLine($"Memory Clock: {memClock} MHz");

int fanSpeed = GetGPUFanSpeed();
Console.WriteLine($"Fan Speed: {fanSpeed} RPM");

float power = GetGPUPowerDraw();
Console.WriteLine($"Power Draw: {power} W");
```

---

## Expected Results

### NVIDIA GPU (with NVAPI):
```
GPU Vendor: 10DE
GPU Temp: 62.0�C
GPU Clock: 1935 MHz
Memory Clock: 7000 MHz
Fan Speed: 1450 RPM
Power Draw: 145.2 W
```

### AMD GPU (with ADL):
```
GPU Vendor: 1002
GPU Temp: 68.0�C
GPU Clock: 2100 MHz
Memory Clock: 1000 MHz
Fan Speed: 2100 RPM
Power Draw: -1.0 W (if not supported)
```

### Intel GPU (built-in):
```
GPU Vendor: 8086
GPU Temp: -1.0�C (not available)
GPU Clock: 0 MHz (not available)
Memory Clock: 0 MHz (not available)
Fan Speed: -1 RPM (not applicable)
Power Draw: -1.0 W (not available)
```

---

## Troubleshooting

### "nvapi.h not found" error:
- Ensure you copied `nvapi.h` to the `DesktopApplication\` folder
- Check the file path matches exactly

### "Unresolved external symbol" linking error:
- Ensure you copied `nvapi64.lib` to the `DesktopApplication\` folder
- Add to linker input if needed: Project Properties > Linker > Input > Additional Dependencies > `nvapi64.lib`

### GPU temperature still returns -1:
- Make sure you uncommented `#define ENABLE_NVIDIA_NVAPI` or `#define ENABLE_AMD_ADL`
- Rebuild the entire solution (Clean > Build)
- Check if your GPU drivers are up to date

### AMD ADL functions not found:
- Ensure AMD drivers are installed
- Check if `atiadlxx.dll` exists in `C:\Windows\System32\`
- Implement the full ADL initialization code (see AMD section)

---

## Next Steps

1. **Choose your GPU vendor** (NVIDIA, AMD, or both)
2. **Download the appropriate SDK(s)**
3. **Copy files to your project**
4. **Uncomment the enable defines**
5. **Build and test!**

Would you like me to help with any specific part of the setup?

---

## Notes

- You can enable **both** NVIDIA and AMD support simultaneously
- The code will auto-detect which GPU is present and use the correct API
- If no vendor SDK is enabled, functions will return `-1` or `0` (unavailable)
- Consider distributing the vendor DLLs with your application for end-users

---

**Created for System Monitor API v1.0**  
GPU monitoring framework ready for SDK integration.
