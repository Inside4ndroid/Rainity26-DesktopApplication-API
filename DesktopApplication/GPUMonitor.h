#pragma once
#include <windows.h>
#include <string>

// ???????????????????????????????????????????????????????????????????????????
// GPU Monitoring - Multi-Vendor Support
// Supports NVIDIA (NVAPI), AMD (ADL), and Intel GPUs
// ???????????????????????????????????????????????????????????????????????????

namespace GPUMonitor
{
    // GPU Vendor enumeration
    enum class GPUVendor
    {
        Unknown = 0,
        NVIDIA = 0x10DE,
        AMD = 0x1002,
        Intel = 0x8086
    };

    // GPU Information structure
    struct GPUInfo
    {
        GPUVendor vendor;
        std::wstring name;
        unsigned int deviceId;
        unsigned int vendorId;
        bool monitoringAvailable;
    };

    // GPU Metrics structure
    struct GPUMetrics
    {
        // Temperature (°C)
        float temperature;
        float hotspotTemperature;  // AMD: junction temp, NVIDIA: hotspot
        
        // Clock Speeds (MHz)
        int gpuClockCurrent;
        int gpuClockMax;
        int memoryClockCurrent;
        int memoryClockMax;
        
        // Usage (%)
        float gpuUsage;
        float memoryUsage;
        
        // Fan (RPM and %)
        int fanSpeedRPM;
        int fanSpeedPercent;
        
        // Power (Watts)
        float powerDraw;
        float powerLimit;
        
        // Voltage (Volts)
        float coreVoltage;
        
        // Memory (MB)
        unsigned long long memoryTotal;
        unsigned long long memoryUsed;
        unsigned long long memoryFree;
        
        // Availability flags
        bool temperatureAvailable;
        bool clocksAvailable;
        bool fanAvailable;
        bool powerAvailable;
        bool voltageAvailable;
    };

    // ???????????????????????????????????????????????????????????????????????
    // Public API Functions
    // ???????????????????????????????????????????????????????????????????????

    /// <summary>
    /// Initialize GPU monitoring. Detects GPU vendor and loads appropriate API.
    /// Must be called before any other GPU monitoring functions.
    /// </summary>
    /// <returns>true if successful, false otherwise</returns>
    bool Initialize();

    /// <summary>
    /// Cleanup GPU monitoring resources.
    /// </summary>
    void Shutdown();

    /// <summary>
    /// Get basic GPU information.
    /// </summary>
    /// <param name="info">Output GPU information structure</param>
    /// <returns>true if successful, false otherwise</returns>
    bool GetGPUInfo(GPUInfo& info);

    /// <summary>
    /// Get current GPU metrics (temperature, clocks, usage, etc.)
    /// </summary>
    /// <param name="metrics">Output GPU metrics structure</param>
    /// <returns>true if successful, false otherwise</returns>
    bool GetGPUMetrics(GPUMetrics& metrics);

    /// <summary>
    /// Get GPU temperature in Celsius.
    /// </summary>
    /// <returns>Temperature in °C, or -1.0 if unavailable</returns>
    float GetTemperature();

    /// <summary>
    /// Get GPU core clock speed in MHz.
    /// </summary>
    /// <returns>Clock speed in MHz, or 0 if unavailable</returns>
    int GetCoreClock();

    /// <summary>
    /// Get GPU memory clock speed in MHz.
    /// </summary>
    /// <returns>Clock speed in MHz, or 0 if unavailable</returns>
    int GetMemoryClock();

    /// <summary>
    /// Get GPU fan speed in RPM.
    /// </summary>
    /// <returns>Fan speed in RPM, or -1 if unavailable</returns>
    int GetFanSpeed();

    /// <summary>
    /// Get GPU fan speed percentage.
    /// </summary>
    /// <returns>Fan speed percentage (0-100), or -1 if unavailable</returns>
    int GetFanSpeedPercent();

    /// <summary>
    /// Get GPU power draw in Watts.
    /// </summary>
    /// <returns>Power draw in Watts, or -1.0 if unavailable</returns>
    float GetPowerDraw();

    /// <summary>
    /// Get GPU core voltage in Volts.
    /// </summary>
    /// <returns>Voltage in Volts, or -1.0 if unavailable</returns>
    float GetVoltage();

    /// <summary>
    /// Get current GPU vendor.
    /// </summary>
    /// <returns>GPU vendor enumeration</returns>
    GPUVendor GetVendor();

} // namespace GPUMonitor
