using UnityEngine;
using UnityEngine.UI;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;

/// <summary>
/// Unity integration for the System Monitor API DLL
/// Attach this script to a GameObject and assign UI elements in the Inspector
/// </summary>
public class SystemMonitorUnity : MonoBehaviour
{
    private const string DLL_NAME = "DesktopApplication";

    #region DLL Imports

    [DllImport(DLL_NAME)]
    private static extern bool Initialize();

    [DllImport(DLL_NAME)]
    private static extern float GetCPUPercentPDH();

    [DllImport(DLL_NAME)]
    private static extern int GetCPUCoreCount();

    [DllImport(DLL_NAME)]
    private static extern float GetCPUCorePercent(int coreIndex);

    [DllImport(DLL_NAME)]
    private static extern int GetCPUFrequency();

    [DllImport(DLL_NAME, CharSet = CharSet.Unicode)]
    private static extern bool GetCPUName(StringBuilder buffer, int bufferSize);

    [DllImport(DLL_NAME)]
    private static extern bool GetMemoryInfo(
        out long memTotal, out long memUsed,
        out long vMemTotal, out long vMemUsed);

    [DllImport(DLL_NAME)]
    private static extern bool GetPageFileUsage(
        out long totalBytes, out long usedBytes);

    [DllImport(DLL_NAME)]
    private static extern bool GetDiskIOStats(
        out long readBytesPerSec, out long writeBytesPerSec);

    [DllImport(DLL_NAME)]
    private static extern bool GetNetworkStats(
        out long bytesSentPerSec, out long bytesRecvPerSec);

    [DllImport(DLL_NAME)]
    private static extern float GetGPUUsagePercent();

    [DllImport(DLL_NAME, CharSet = CharSet.Unicode)]
    private static extern bool GetGPUName(StringBuilder buffer, int bufferSize);

    [DllImport(DLL_NAME)]
    private static extern bool GetBatteryStatus(
        out int percentage, out bool isCharging, out int remainingMinutes);

    [DllImport(DLL_NAME)]
    private static extern long GetSystemUptime();

    [DllImport(DLL_NAME)]
    private static extern int GetProcessCount();

    [DllImport(DLL_NAME)]
    private static extern int GetThreadCount();

    #endregion

    #region Public UI References

    [Header("CPU UI")]
    public Text cpuUsageText;
    public Slider cpuUsageSlider;
    public Text cpuNameText;
    public Text[] coreUsageTexts; // Array for per-core display

    [Header("Memory UI")]
    public Text ramUsageText;
    public Slider ramUsageSlider;
    public Text vramUsageText;
    public Slider vramUsageSlider;

    [Header("GPU UI")]
    public Text gpuUsageText;
    public Slider gpuUsageSlider;
    public Text gpuNameText;

    [Header("Disk & Network UI")]
    public Text diskReadText;
    public Text diskWriteText;
    public Text netUploadText;
    public Text netDownloadText;

    [Header("System Info UI")]
    public Text systemInfoText;
    public Text uptimeText;
    public Text processCountText;

    [Header("Battery UI")]
    public Text batteryText;
    public Slider batterySlider;

    [Header("Settings")]
    [Tooltip("Update interval in seconds")]
    public float updateInterval = 1.0f;

    [Tooltip("Enable per-core CPU monitoring (may impact performance)")]
    public bool showPerCoreUsage = false;

    [Tooltip("Enable detailed logging")]
    public bool enableLogging = false;

    #endregion

    #region Private Fields

    private float updateTimer = 0f;
    private bool isInitialized = false;
    private int coreCount = 0;

    // Cached data for smooth updates
    private float currentCPU = 0f;
    private float currentGPU = 0f;
    private float currentRAM = 0f;
    private float currentVRAM = 0f;

    #endregion

    void Start()
    {
        // Initialize the DLL
        isInitialized = Initialize();

        if (!isInitialized)
        {
            Debug.LogError("Failed to initialize System Monitor API!");
            enabled = false;
            return;
        }

        Debug.Log("System Monitor API initialized successfully!");

        // Get static information once
        coreCount = GetCPUCoreCount();
        
        // Display system information
        DisplayStaticInfo();
        
        // Initial update
        UpdateMonitoringData();
    }

    void Update()
    {
        if (!isInitialized) return;

        updateTimer += Time.deltaTime;

        if (updateTimer >= updateInterval)
        {
            updateTimer = 0f;
            UpdateMonitoringData();
        }
    }

    private void DisplayStaticInfo()
    {
        // CPU Name
        if (cpuNameText != null)
        {
            var cpuName = new StringBuilder(256);
            if (GetCPUName(cpuName, 256))
            {
                int frequency = GetCPUFrequency();
                cpuNameText.text = $"{cpuName} ({coreCount} cores @ {frequency} MHz)";
            }
        }

        // GPU Name
        if (gpuNameText != null)
        {
            var gpuName = new StringBuilder(256);
            if (GetGPUName(gpuName, 256))
            {
                gpuNameText.text = gpuName.ToString();
            }
        }

        if (enableLogging)
        {
            Debug.Log($"CPU Cores: {coreCount}");
        }
    }

    private void UpdateMonitoringData()
    {
        // Update CPU
        UpdateCPU();

        // Update Memory
        UpdateMemory();

        // Update GPU
        UpdateGPU();

        // Update Disk I/O
        UpdateDiskIO();

        // Update Network
        UpdateNetwork();

        // Update System Info
        UpdateSystemInfo();

        // Update Battery
        UpdateBattery();
    }

    private void UpdateCPU()
    {
        float cpuUsage = GetCPUPercentPDH();
        currentCPU = Mathf.Lerp(currentCPU, cpuUsage, 0.5f); // Smooth transition

        if (cpuUsageText != null)
        {
            cpuUsageText.text = $"CPU: {currentCPU:F1}%";
        }

        if (cpuUsageSlider != null)
        {
            cpuUsageSlider.value = currentCPU / 100f;
        }

        // Per-core usage
        if (showPerCoreUsage && coreUsageTexts != null && coreUsageTexts.Length > 0)
        {
            int displayCores = Mathf.Min(coreCount, coreUsageTexts.Length);
            for (int i = 0; i < displayCores; i++)
            {
                if (coreUsageTexts[i] != null)
                {
                    float coreUsage = GetCPUCorePercent(i);
                    coreUsageTexts[i].text = $"Core {i}: {coreUsage:F1}%";
                }
            }
        }

        if (enableLogging)
        {
            Debug.Log($"CPU Usage: {currentCPU:F2}%");
        }
    }

    private void UpdateMemory()
    {
        if (GetMemoryInfo(out long memTotal, out long memUsed,
                         out long vMemTotal, out long vMemUsed))
        {
            float ramGB = memUsed / 1073741824f;
            float ramTotalGB = memTotal / 1073741824f;
            float ramPercent = (memUsed * 100f) / memTotal;
            currentRAM = Mathf.Lerp(currentRAM, ramPercent, 0.5f);

            if (ramUsageText != null)
            {
                ramUsageText.text = $"RAM: {ramGB:F2} / {ramTotalGB:F2} GB ({currentRAM:F1}%)";
            }

            if (ramUsageSlider != null)
            {
                ramUsageSlider.value = currentRAM / 100f;
            }

            // VRAM
            if (vMemTotal > 0)
            {
                float vramGB = vMemUsed / 1073741824f;
                float vramTotalGB = vMemTotal / 1073741824f;
                float vramPercent = (vMemUsed * 100f) / vMemTotal;
                currentVRAM = Mathf.Lerp(currentVRAM, vramPercent, 0.5f);

                if (vramUsageText != null)
                {
                    vramUsageText.text = $"VRAM: {vramGB:F2} / {vramTotalGB:F2} GB ({currentVRAM:F1}%)";
                }

                if (vramUsageSlider != null)
                {
                    vramUsageSlider.value = currentVRAM / 100f;
                }
            }
        }
    }

    private void UpdateGPU()
    {
        float gpuUsage = GetGPUUsagePercent();
        currentGPU = Mathf.Lerp(currentGPU, gpuUsage, 0.5f);

        if (gpuUsageText != null)
        {
            gpuUsageText.text = $"GPU: {currentGPU:F1}%";
        }

        if (gpuUsageSlider != null)
        {
            gpuUsageSlider.value = currentGPU / 100f;
        }
    }

    private void UpdateDiskIO()
    {
        if (GetDiskIOStats(out long diskRead, out long diskWrite))
        {
            if (diskReadText != null)
            {
                float readMB = diskRead / 1048576f;
                diskReadText.text = $"Disk Read: {readMB:F2} MB/s";
            }

            if (diskWriteText != null)
            {
                float writeMB = diskWrite / 1048576f;
                diskWriteText.text = $"Disk Write: {writeMB:F2} MB/s";
            }
        }
    }

    private void UpdateNetwork()
    {
        if (GetNetworkStats(out long netSent, out long netRecv))
        {
            if (netUploadText != null)
            {
                float uploadMB = netSent / 1048576f;
                netUploadText.text = $"Upload: {uploadMB:F2} MB/s";
            }

            if (netDownloadText != null)
            {
                float downloadMB = netRecv / 1048576f;
                netDownloadText.text = $"Download: {downloadMB:F2} MB/s";
            }
        }
    }

    private void UpdateSystemInfo()
    {
        // Uptime
        if (uptimeText != null)
        {
            long uptime = GetSystemUptime();
            int days = (int)(uptime / 86400);
            int hours = (int)((uptime % 86400) / 3600);
            int minutes = (int)((uptime % 3600) / 60);
            uptimeText.text = $"Uptime: {days}d {hours}h {minutes}m";
        }

        // Process count
        if (processCountText != null)
        {
            int processCount = GetProcessCount();
            int threadCount = GetThreadCount();
            processCountText.text = $"Processes: {processCount} | Threads: {threadCount}";
        }
    }

    private void UpdateBattery()
    {
        if (GetBatteryStatus(out int percentage, out bool isCharging, out int remainingMinutes))
        {
            if (percentage >= 0) // Battery present
            {
                if (batteryText != null)
                {
                    string status = isCharging ? "? Charging" : "?? Discharging";
                    string timeRemaining = "";
                    
                    if (!isCharging && remainingMinutes > 0)
                    {
                        int hours = remainingMinutes / 60;
                        int mins = remainingMinutes % 60;
                        timeRemaining = $" ({hours}h {mins}m remaining)";
                    }

                    batteryText.text = $"Battery: {percentage}% {status}{timeRemaining}";
                }

                if (batterySlider != null)
                {
                    batterySlider.value = percentage / 100f;
                }
            }
            else if (batteryText != null)
            {
                batteryText.text = "No battery detected";
            }
        }
    }

    void OnDestroy()
    {
        // Cleanup if needed
        if (enableLogging)
        {
            Debug.Log("System Monitor cleanup");
        }
    }

    #region Public API for other scripts

    /// <summary>
    /// Get current CPU usage (0-100)
    /// </summary>
    public float GetCurrentCPUUsage()
    {
        return currentCPU;
    }

    /// <summary>
    /// Get current RAM usage (0-100)
    /// </summary>
    public float GetCurrentRAMUsage()
    {
        return currentRAM;
    }

    /// <summary>
    /// Get current GPU usage (0-100)
    /// </summary>
    public float GetCurrentGPUUsage()
    {
        return currentGPU;
    }

    /// <summary>
    /// Get memory info in GB
    /// </summary>
    public bool GetMemoryGB(out float usedGB, out float totalGB)
    {
        if (GetMemoryInfo(out long memTotal, out long memUsed,
                         out long vMemTotal, out long vMemUsed))
        {
            usedGB = memUsed / 1073741824f;
            totalGB = memTotal / 1073741824f;
            return true;
        }

        usedGB = 0f;
        totalGB = 0f;
        return false;
    }

    #endregion
}
