using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace SystemMonitorAdvanced
{
    /// <summary>
    /// Advanced System Monitor with historical data tracking and statistics
    /// </summary>
    public class AdvancedSystemMonitor
    {
        private const string DLL_NAME = "DesktopApplication.dll";

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
        private static extern bool GetMemoryInfo(
            out long memTotal, out long memUsed,
            out long vMemTotal, out long vMemUsed);

        [DllImport(DLL_NAME)]
        private static extern bool GetDiskIOStats(
            out long readBytesPerSec, out long writeBytesPerSec);

        [DllImport(DLL_NAME)]
        private static extern bool GetNetworkStats(
            out long bytesSentPerSec, out long bytesRecvPerSec);

        [DllImport(DLL_NAME)]
        private static extern float GetGPUUsagePercent();

        #endregion

        #region Data Classes

        public class MonitoringSnapshot
        {
            public DateTime Timestamp { get; set; }
            public float CPUUsage { get; set; }
            public float[] CoreUsages { get; set; }
            public long RAMUsed { get; set; }
            public long RAMTotal { get; set; }
            public long VRAMUsed { get; set; }
            public long VRAMTotal { get; set; }
            public long DiskReadRate { get; set; }
            public long DiskWriteRate { get; set; }
            public long NetworkUploadRate { get; set; }
            public long NetworkDownloadRate { get; set; }
            public float GPUUsage { get; set; }

            public float RAMUsagePercent => RAMTotal > 0 ? (RAMUsed * 100f) / RAMTotal : 0f;
            public float VRAMUsagePercent => VRAMTotal > 0 ? (VRAMUsed * 100f) / VRAMTotal : 0f;
        }

        public class HistoricalStats
        {
            public float Average { get; set; }
            public float Min { get; set; }
            public float Max { get; set; }
            public float Current { get; set; }
        }

        #endregion

        #region Fields

        private readonly int maxHistorySize;
        private readonly List<MonitoringSnapshot> history;
        private readonly int coreCount;
        private bool isInitialized;

        #endregion

        #region Constructor

        public AdvancedSystemMonitor(int historySize = 60)
        {
            maxHistorySize = historySize;
            history = new List<MonitoringSnapshot>(historySize);
            
            isInitialized = Initialize();
            if (!isInitialized)
            {
                throw new InvalidOperationException("Failed to initialize System Monitor API");
            }

            coreCount = GetCPUCoreCount();
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Takes a snapshot of current system state
        /// </summary>
        public MonitoringSnapshot TakeSnapshot()
        {
            if (!isInitialized)
                return null;

            var snapshot = new MonitoringSnapshot
            {
                Timestamp = DateTime.Now,
                CPUUsage = GetCPUPercentPDH(),
                CoreUsages = new float[coreCount],
                GPUUsage = GetGPUUsagePercent()
            };

            // Get per-core usage
            for (int i = 0; i < coreCount; i++)
            {
                snapshot.CoreUsages[i] = GetCPUCorePercent(i);
            }

            // Memory
            if (GetMemoryInfo(out long memTotal, out long memUsed,
                             out long vMemTotal, out long vMemUsed))
            {
                snapshot.RAMUsed = memUsed;
                snapshot.RAMTotal = memTotal;
                snapshot.VRAMUsed = vMemUsed;
                snapshot.VRAMTotal = vMemTotal;
            }

            // Disk I/O
            if (GetDiskIOStats(out long diskRead, out long diskWrite))
            {
                snapshot.DiskReadRate = diskRead;
                snapshot.DiskWriteRate = diskWrite;
            }

            // Network
            if (GetNetworkStats(out long netSent, out long netRecv))
            {
                snapshot.NetworkUploadRate = netSent;
                snapshot.NetworkDownloadRate = netRecv;
            }

            // Add to history
            history.Add(snapshot);
            if (history.Count > maxHistorySize)
            {
                history.RemoveAt(0);
            }

            return snapshot;
        }

        /// <summary>
        /// Gets historical statistics for CPU usage
        /// </summary>
        public HistoricalStats GetCPUStats()
        {
            if (history.Count == 0) return null;

            var values = history.Select(h => h.CPUUsage).ToList();
            return new HistoricalStats
            {
                Current = values.Last(),
                Average = values.Average(),
                Min = values.Min(),
                Max = values.Max()
            };
        }

        /// <summary>
        /// Gets historical statistics for RAM usage percentage
        /// </summary>
        public HistoricalStats GetRAMStats()
        {
            if (history.Count == 0) return null;

            var values = history.Select(h => h.RAMUsagePercent).ToList();
            return new HistoricalStats
            {
                Current = values.Last(),
                Average = values.Average(),
                Min = values.Min(),
                Max = values.Max()
            };
        }

        /// <summary>
        /// Gets historical statistics for GPU usage
        /// </summary>
        public HistoricalStats GetGPUStats()
        {
            if (history.Count == 0) return null;

            var values = history.Select(h => h.GPUUsage).ToList();
            return new HistoricalStats
            {
                Current = values.Last(),
                Average = values.Average(),
                Min = values.Min(),
                Max = values.Max()
            };
        }

        /// <summary>
        /// Gets historical disk I/O statistics (MB/s)
        /// </summary>
        public (HistoricalStats Read, HistoricalStats Write) GetDiskIOStats()
        {
            if (history.Count == 0) return (null, null);

            var readValues = history.Select(h => h.DiskReadRate / 1048576f).ToList();
            var writeValues = history.Select(h => h.DiskWriteRate / 1048576f).ToList();

            var readStats = new HistoricalStats
            {
                Current = readValues.Last(),
                Average = readValues.Average(),
                Min = readValues.Min(),
                Max = readValues.Max()
            };

            var writeStats = new HistoricalStats
            {
                Current = writeValues.Last(),
                Average = writeValues.Average(),
                Min = writeValues.Min(),
                Max = writeValues.Max()
            };

            return (readStats, writeStats);
        }

        /// <summary>
        /// Gets historical network statistics (MB/s)
        /// </summary>
        public (HistoricalStats Upload, HistoricalStats Download) GetNetworkStats()
        {
            if (history.Count == 0) return (null, null);

            var uploadValues = history.Select(h => h.NetworkUploadRate / 1048576f).ToList();
            var downloadValues = history.Select(h => h.NetworkDownloadRate / 1048576f).ToList();

            var uploadStats = new HistoricalStats
            {
                Current = uploadValues.Last(),
                Average = uploadValues.Average(),
                Min = uploadValues.Min(),
                Max = uploadValues.Max()
            };

            var downloadStats = new HistoricalStats
            {
                Current = downloadValues.Last(),
                Average = downloadValues.Average(),
                Min = downloadValues.Min(),
                Max = downloadValues.Max()
            };

            return (uploadStats, downloadStats);
        }

        /// <summary>
        /// Gets all historical snapshots
        /// </summary>
        public IReadOnlyList<MonitoringSnapshot> GetHistory()
        {
            return history.AsReadOnly();
        }

        /// <summary>
        /// Clears historical data
        /// </summary>
        public void ClearHistory()
        {
            history.Clear();
        }

        /// <summary>
        /// Generates an ASCII graph for the specified metric
        /// </summary>
        public string GenerateASCIIGraph(Func<MonitoringSnapshot, float> selector, 
                                        int width = 60, 
                                        int height = 10,
                                        string title = "Graph")
        {
            if (history.Count < 2) return "Insufficient data";

            var values = history.Select(selector).ToList();
            float max = values.Max();
            float min = values.Min();
            float range = max - min;
            if (range < 0.01f) range = 1f; // Prevent division by zero

            var sb = new StringBuilder();
            sb.AppendLine($"??? {title} (Last {history.Count} samples) ??");
            sb.AppendLine($"? Max: {max:F2}  Min: {min:F2}  Avg: {values.Average():F2}");
            sb.AppendLine("?" + new string('?', width));

            // Draw graph from top to bottom
            for (int row = height - 1; row >= 0; row--)
            {
                sb.Append("?");
                float threshold = min + (range * row / (height - 1));

                int startIndex = Math.Max(0, values.Count - width);
                for (int i = startIndex; i < values.Count; i++)
                {
                    float value = values[i];
                    if (value >= threshold)
                    {
                        sb.Append('?');
                    }
                    else
                    {
                        sb.Append(' ');
                    }
                }
                sb.AppendLine();
            }

            sb.AppendLine("?" + new string('?', width));
            return sb.ToString();
        }

        /// <summary>
        /// Generates a report with all statistics
        /// </summary>
        public string GenerateReport()
        {
            if (history.Count == 0) return "No data available";

            var sb = new StringBuilder();
            sb.AppendLine("???????????????????????????????????????????????????????????");
            sb.AppendLine("              SYSTEM MONITORING REPORT");
            sb.AppendLine($"              {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            sb.AppendLine("???????????????????????????????????????????????????????????");
            sb.AppendLine();

            // CPU Statistics
            var cpuStats = GetCPUStats();
            sb.AppendLine("CPU Statistics:");
            sb.AppendLine($"  Current:  {cpuStats.Current,6:F2}%");
            sb.AppendLine($"  Average:  {cpuStats.Average,6:F2}%");
            sb.AppendLine($"  Min/Max:  {cpuStats.Min,6:F2}% / {cpuStats.Max:F2}%");
            sb.AppendLine();

            // RAM Statistics
            var ramStats = GetRAMStats();
            sb.AppendLine("RAM Statistics:");
            sb.AppendLine($"  Current:  {ramStats.Current,6:F2}%");
            sb.AppendLine($"  Average:  {ramStats.Average,6:F2}%");
            sb.AppendLine($"  Min/Max:  {ramStats.Min,6:F2}% / {ramStats.Max:F2}%");
            sb.AppendLine();

            // GPU Statistics
            var gpuStats = GetGPUStats();
            if (gpuStats.Max > 0)
            {
                sb.AppendLine("GPU Statistics:");
                sb.AppendLine($"  Current:  {gpuStats.Current,6:F2}%");
                sb.AppendLine($"  Average:  {gpuStats.Average,6:F2}%");
                sb.AppendLine($"  Min/Max:  {gpuStats.Min,6:F2}% / {gpuStats.Max:F2}%");
                sb.AppendLine();
            }

            // Disk I/O
            var (diskRead, diskWrite) = GetDiskIOStats();
            sb.AppendLine("Disk I/O Statistics (MB/s):");
            sb.AppendLine($"  Read:     Cur {diskRead.Current,6:F2}  Avg {diskRead.Average,6:F2}  Max {diskRead.Max,6:F2}");
            sb.AppendLine($"  Write:    Cur {diskWrite.Current,6:F2}  Avg {diskWrite.Average,6:F2}  Max {diskWrite.Max,6:F2}");
            sb.AppendLine();

            // Network
            var (netUp, netDown) = GetNetworkStats();
            sb.AppendLine("Network Statistics (MB/s):");
            sb.AppendLine($"  Upload:   Cur {netUp.Current,6:F2}  Avg {netUp.Average,6:F2}  Max {netUp.Max,6:F2}");
            sb.AppendLine($"  Download: Cur {netDown.Current,6:F2}  Avg {netDown.Average,6:F2}  Max {netDown.Max,6:F2}");
            sb.AppendLine();

            sb.AppendLine("???????????????????????????????????????????????????????????");

            return sb.ToString();
        }

        #endregion
    }
}
