using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace SystemMonitorExample
{
    /// <summary>
    /// Example C# wrapper and usage demonstration for the System Monitor API DLL
    /// </summary>
    public class SystemMonitorAPI
    {
        private const string DLL_NAME = "DesktopApplication.dll";

        // Initialization
        [DllImport(DLL_NAME)]
        public static extern bool Initialize();

        // CPU Monitoring
        [DllImport(DLL_NAME)]
        public static extern float GetCPUPercentPDH();

        [DllImport(DLL_NAME)]
        public static extern int GetCPUCoreCount();

        [DllImport(DLL_NAME)]
        public static extern float GetCPUCorePercent(int coreIndex);

        [DllImport(DLL_NAME)]
        public static extern int GetCPUFrequency();

        [DllImport(DLL_NAME, CharSet = CharSet.Unicode)]
        public static extern bool GetCPUName(StringBuilder buffer, int bufferSize);

        [DllImport(DLL_NAME)]
        public static extern float GetCPUTemperature();

        // Memory Monitoring
        [DllImport(DLL_NAME)]
        public static extern bool GetMemoryInfo(
            out long memTotal, out long memUsed,
            out long vMemTotal, out long vMemUsed);

        [DllImport(DLL_NAME)]
        public static extern bool GetPageFileUsage(
            out long totalBytes, out long usedBytes);

        // Disk Monitoring
        [DllImport(DLL_NAME)]
        public static extern bool GetDiskIOStats(
            out long readBytesPerSec, out long writeBytesPerSec);

        [DllImport(DLL_NAME, CharSet = CharSet.Unicode)]
        public static extern bool GetDiskSpace(
            char driveLetter, out long totalBytes, out long freeBytes);

        // Network Monitoring
        [DllImport(DLL_NAME)]
        public static extern bool GetNetworkStats(
            out long bytesSentPerSec, out long bytesRecvPerSec);

        // GPU Monitoring
        [DllImport(DLL_NAME)]
        public static extern float GetGPUUsagePercent();

        [DllImport(DLL_NAME, CharSet = CharSet.Unicode)]
        public static extern bool GetGPUName(StringBuilder buffer, int bufferSize);

        [DllImport(DLL_NAME)]
        public static extern float GetGPUTemperature();

        [DllImport(DLL_NAME)]
        public static extern int GetGPUClockSpeed();

        // Battery & Power
        [DllImport(DLL_NAME)]
        public static extern bool GetBatteryStatus(
            out int percentage, out bool isCharging, out int remainingMinutes);

        // System Information
        [DllImport(DLL_NAME)]
        public static extern long GetSystemUptime();

        [DllImport(DLL_NAME)]
        public static extern int GetProcessCount();

        [DllImport(DLL_NAME)]
        public static extern int GetThreadCount();

        [DllImport(DLL_NAME)]
        public static extern int GetHandleCount();

        [DllImport(DLL_NAME, CharSet = CharSet.Unicode)]
        public static extern bool GetSystemInfoString(StringBuilder buffer, int bufferSize);
    }

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("??????????????????????????????????????????????????????????????");
            Console.WriteLine("?         System Monitor API - Example Application          ?");
            Console.WriteLine("??????????????????????????????????????????????????????????????");
            Console.WriteLine();

            // Initialize the monitoring system
            if (!SystemMonitorAPI.Initialize())
            {
                Console.WriteLine("? Failed to initialize System Monitor API!");
                Console.WriteLine("Press any key to exit...");
                Console.ReadKey();
                return;
            }

            Console.WriteLine("? System Monitor API initialized successfully!");
            Console.WriteLine();

            // Display system information once
            DisplaySystemInfo();
            Console.WriteLine();

            // Continuous monitoring loop
            Console.WriteLine("Monitoring system (Press ESC to exit)...");
            Console.WriteLine(new string('?', 60));
            Console.WriteLine();

            while (true)
            {
                if (Console.KeyAvailable && Console.ReadKey(true).Key == ConsoleKey.Escape)
                    break;

                Console.SetCursorPosition(0, Console.CursorTop - GetOutputLineCount());
                DisplayMonitoringData();

                Thread.Sleep(1000); // Update every second
            }

            Console.WriteLine("\nExiting...");
        }

        static void DisplaySystemInfo()
        {
            Console.WriteLine("??? SYSTEM INFORMATION ???");

            // System Info
            var sysInfo = new StringBuilder(512);
            if (SystemMonitorAPI.GetSystemInfoString(sysInfo, 512))
            {
                Console.WriteLine($"OS: {sysInfo}");
            }

            // CPU Info
            var cpuName = new StringBuilder(256);
            if (SystemMonitorAPI.GetCPUName(cpuName, 256))
            {
                Console.WriteLine($"CPU: {cpuName}");
            }

            int coreCount = SystemMonitorAPI.GetCPUCoreCount();
            int frequency = SystemMonitorAPI.GetCPUFrequency();
            Console.WriteLine($"CPU Cores: {coreCount} threads @ {frequency} MHz");

            // GPU Info
            var gpuName = new StringBuilder(256);
            if (SystemMonitorAPI.GetGPUName(gpuName, 256))
            {
                Console.WriteLine($"GPU: {gpuName}");
            }

            // Memory Info
            if (SystemMonitorAPI.GetMemoryInfo(out long memTotal, out long memUsed,
                                              out long vMemTotal, out long vMemUsed))
            {
                Console.WriteLine($"RAM: {memTotal / 1073741824.0:F2} GB");
                Console.WriteLine($"VRAM: {vMemTotal / 1073741824.0:F2} GB");
            }
        }

        static void DisplayMonitoringData()
        {
            // CPU Usage
            float cpuUsage = SystemMonitorAPI.GetCPUPercentPDH();
            Console.WriteLine($"CPU Usage:      {cpuUsage,6:F2}% {GetUsageBar(cpuUsage)}");

            // Per-Core Usage (show first 8 cores if available)
            int coreCount = SystemMonitorAPI.GetCPUCoreCount();
            int displayCores = Math.Min(coreCount, 8);
            for (int i = 0; i < displayCores; i++)
            {
                float coreUsage = SystemMonitorAPI.GetCPUCorePercent(i);
                Console.WriteLine($"  Core {i,2}:      {coreUsage,6:F2}% {GetUsageBar(coreUsage, 20)}");
            }
            if (coreCount > 8)
            {
                Console.WriteLine($"  ... and {coreCount - 8} more cores");
            }

            // Memory Usage
            if (SystemMonitorAPI.GetMemoryInfo(out long memTotal, out long memUsed,
                                              out long vMemTotal, out long vMemUsed))
            {
                float ramPercent = (memUsed * 100.0f) / memTotal;
                Console.WriteLine($"RAM Usage:      {memUsed / 1073741824.0,6:F2} GB / {memTotal / 1073741824.0:F2} GB ({ramPercent:F1}%) {GetUsageBar(ramPercent)}");

                if (vMemTotal > 0)
                {
                    float vramPercent = (vMemUsed * 100.0f) / vMemTotal;
                    Console.WriteLine($"VRAM Usage:     {vMemUsed / 1073741824.0,6:F2} GB / {vMemTotal / 1073741824.0:F2} GB ({vramPercent:F1}%) {GetUsageBar(vramPercent)}");
                }
            }

            // GPU Usage
            float gpuUsage = SystemMonitorAPI.GetGPUUsagePercent();
            if (gpuUsage > 0)
            {
                Console.WriteLine($"GPU Usage:      {gpuUsage,6:F2}% {GetUsageBar(gpuUsage)}");
            }

            // Disk I/O
            if (SystemMonitorAPI.GetDiskIOStats(out long diskRead, out long diskWrite))
            {
                Console.WriteLine($"Disk Read:      {diskRead / 1048576.0,6:F2} MB/s");
                Console.WriteLine($"Disk Write:     {diskWrite / 1048576.0,6:F2} MB/s");
            }

            // Network
            if (SystemMonitorAPI.GetNetworkStats(out long netSent, out long netRecv))
            {
                Console.WriteLine($"Net Upload:     {netSent / 1048576.0,6:F2} MB/s");
                Console.WriteLine($"Net Download:   {netRecv / 1048576.0,6:F2} MB/s");
            }

            // System Stats
            int processCount = SystemMonitorAPI.GetProcessCount();
            int threadCount = SystemMonitorAPI.GetThreadCount();
            Console.WriteLine($"Processes:      {processCount,6}    Threads: {threadCount}");

            // Uptime
            long uptime = SystemMonitorAPI.GetSystemUptime();
            TimeSpan ts = TimeSpan.FromSeconds(uptime);
            Console.WriteLine($"Uptime:         {ts.Days}d {ts.Hours}h {ts.Minutes}m {ts.Seconds}s");

            // Battery (if available)
            if (SystemMonitorAPI.GetBatteryStatus(out int batteryPercent, out bool isCharging, out int remainingMin))
            {
                if (batteryPercent >= 0)
                {
                    string chargingStatus = isCharging ? "?Charging" : "??Discharging";
                    Console.WriteLine($"Battery:        {batteryPercent,3}% {chargingStatus} {GetUsageBar(batteryPercent)}");
                }
            }

            Console.WriteLine();
        }

        static string GetUsageBar(float percent, int width = 30)
        {
            int filled = (int)(percent / 100.0f * width);
            filled = Math.Max(0, Math.Min(width, filled));
            
            string bar = "[" + new string('?', filled) + new string('?', width - filled) + "]";
            
            // Color coding
            if (percent >= 90) return bar; // No color for high usage in console
            return bar;
        }

        static int GetOutputLineCount()
        {
            // Calculate number of output lines for cursor repositioning
            int coreCount = SystemMonitorAPI.GetCPUCoreCount();
            int displayCores = Math.Min(coreCount, 8);
            
            int lines = 1; // CPU Usage
            lines += displayCores; // Per-core usage
            if (coreCount > 8) lines++; // "... and X more cores"
            lines += 2; // RAM and VRAM
            lines += 1; // GPU (may be 0 but we account for it)
            lines += 2; // Disk I/O
            lines += 2; // Network
            lines += 1; // Process count
            lines += 1; // Uptime
            lines += 1; // Battery (may not show but we account for it)
            lines += 1; // Empty line

            return lines;
        }
    }
}
