#pragma once

// ???????????????????????????????????????????????????????????????????????????
// System Monitor API - Comprehensive Windows System Monitoring DLL
// ???????????????????????????????????????????????????????????????????????????
// This DLL provides extensive system monitoring capabilities including:
// - CPU usage (overall and per-core)
// - Memory usage (RAM and VRAM)
// - Disk I/O and space
// - Network statistics
// - GPU information and usage
// - Battery status
// - System information and uptime
// - Process/thread/handle counts
// - Temperature monitoring (placeholders for hardware-specific implementations)
// ???????????????????????????????????????????????????????????????????????????

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes the monitoring system. Must be called once at startup.
/// Sets up PDH queries and counters for all monitoring functions.
/// </summary>
/// <returns>true if initialization succeeded, false otherwise</returns>
__declspec(dllexport) bool Initialize();

// ---------------------------------------------------------------------------
// CPU Monitoring
// ---------------------------------------------------------------------------

/// <summary>
/// Gets overall CPU utilization percentage (0-100%), frequency-adjusted.
/// Matches Task Manager's display.
/// </summary>
/// <returns>CPU usage percentage as float (0.0 - 100.0)</returns>
__declspec(dllexport) float GetCPUPercentPDH();

/// <summary>
/// Legacy function: Returns CPU usage as integer (percent * 1,000,000).
/// </summary>
/// <returns>CPU usage scaled by 1,000,000</returns>
__declspec(dllexport) int GetCPUPercent();

/// <summary>
/// Gets the number of logical CPU cores (threads) available.
/// </summary>
/// <returns>Number of logical processors</returns>
__declspec(dllexport) int GetCPUCoreCount();

/// <summary>
/// Gets CPU usage for a specific core (0-100%).
/// </summary>
/// <param name="coreIndex">Zero-based core index (0 to GetCPUCoreCount()-1)</param>
/// <returns>Core usage percentage (0.0 - 100.0)</returns>
__declspec(dllexport) float GetCPUCorePercent(int coreIndex);

/// <summary>
/// Gets current CPU base frequency in MHz.
/// </summary>
/// <returns>CPU frequency in MHz, or 0 if unavailable</returns>
__declspec(dllexport) int GetCPUFrequency();

/// <summary>
/// Gets CPU brand string (e.g., "Intel Core i7-9700K").
/// </summary>
/// <param name="buffer">Wide-character buffer to receive the name</param>
/// <param name="bufferSize">Size of buffer in characters (recommended: 256)</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetCPUName(wchar_t* buffer, int bufferSize);

/// <summary>
/// Gets CPU temperature in Celsius.
/// NOTE: Currently a placeholder. Requires WMI or hardware-specific drivers.
/// </summary>
/// <returns>Temperature in Celsius, or -1.0 if not available</returns>
__declspec(dllexport) float GetCPUTemperature();

// ---------------------------------------------------------------------------
// Memory Monitoring
// ---------------------------------------------------------------------------

/// <summary>
/// Gets physical RAM and GPU VRAM usage information.
/// </summary>
/// <param name="memTotal">Output: Total physical RAM in bytes</param>
/// <param name="memUsed">Output: Used physical RAM in bytes</param>
/// <param name="vMemTotal">Output: Total GPU VRAM in bytes</param>
/// <param name="vMemUsed">Output: Used GPU VRAM in bytes</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetMemoryInfo(long long* memTotal, long long* memUsed,
                                          long long* vMemTotal, long long* vMemUsed);

/// <summary>
/// Gets page file (virtual memory) usage information.
/// </summary>
/// <param name="totalBytes">Output: Total page file size in bytes</param>
/// <param name="usedBytes">Output: Used page file size in bytes</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetPageFileUsage(long long* totalBytes, long long* usedBytes);

// ---------------------------------------------------------------------------
// Disk Monitoring
// ---------------------------------------------------------------------------

/// <summary>
/// Gets disk I/O statistics (read/write throughput).
/// </summary>
/// <param name="readBytesPerSec">Output: Bytes read per second across all disks</param>
/// <param name="writeBytesPerSec">Output: Bytes written per second across all disks</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetDiskIOStats(long long* readBytesPerSec, long long* writeBytesPerSec);

/// <summary>
/// Gets total and free space for a specific drive.
/// </summary>
/// <param name="driveLetter">Drive letter (e.g., 'C')</param>
/// <param name="totalBytes">Output: Total drive capacity in bytes</param>
/// <param name="freeBytes">Output: Free space in bytes</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetDiskSpace(wchar_t driveLetter, long long* totalBytes, long long* freeBytes);

// ---------------------------------------------------------------------------
// Network Monitoring
// ---------------------------------------------------------------------------

/// <summary>
/// Gets network throughput statistics.
/// </summary>
/// <param name="bytesSentPerSec">Output: Bytes sent per second across all interfaces</param>
/// <param name="bytesRecvPerSec">Output: Bytes received per second across all interfaces</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetNetworkStats(long long* bytesSentPerSec, long long* bytesRecvPerSec);

// ---------------------------------------------------------------------------
// GPU Monitoring
// ---------------------------------------------------------------------------

/// <summary>
/// Gets GPU utilization percentage (0-100%).
/// NOTE: May not be available on all systems/GPUs.
/// </summary>
/// <returns>GPU usage percentage (0.0 - 100.0), or 0.0 if unavailable</returns>
__declspec(dllexport) float GetGPUUsagePercent();

/// <summary>
/// Gets GPU name/description string.
/// </summary>
/// <param name="buffer">Wide-character buffer to receive the name</param>
/// <param name="bufferSize">Size of buffer in characters (recommended: 256)</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetGPUName(wchar_t* buffer, int bufferSize);

/// <summary>
/// Gets GPU temperature in Celsius.
/// NOTE: Currently a placeholder. Requires vendor-specific APIs (NVAPI, ADL, etc.)
/// </summary>
/// <returns>Temperature in Celsius, or -1.0 if not available</returns>
__declspec(dllexport) float GetGPUTemperature();

/// <summary>
/// Gets GPU clock speed in MHz.
/// NOTE: Currently a placeholder. Requires vendor-specific APIs.
/// </summary>
/// <returns>Clock speed in MHz, or 0 if not available</returns>
__declspec(dllexport) int GetGPUClockSpeed();

// ---------------------------------------------------------------------------
// Battery & Power
// ---------------------------------------------------------------------------

/// <summary>
/// Gets battery status information.
/// </summary>
/// <param name="percentage">Output: Battery charge percentage (0-100), or -1 if no battery</param>
/// <param name="isCharging">Output: true if AC power connected and charging</param>
/// <param name="remainingMinutes">Output: Estimated minutes remaining, or -1 if unknown</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetBatteryStatus(int* percentage, bool* isCharging, int* remainingMinutes);

// ---------------------------------------------------------------------------
// System Information
// ---------------------------------------------------------------------------

/// <summary>
/// Gets system uptime in seconds since last boot.
/// </summary>
/// <returns>Uptime in seconds</returns>
__declspec(dllexport) long long GetSystemUptime();

/// <summary>
/// Gets the number of currently running processes.
/// </summary>
/// <returns>Process count</returns>
__declspec(dllexport) int GetProcessCount();

/// <summary>
/// Gets the total number of threads across all processes.
/// </summary>
/// <returns>Thread count</returns>
__declspec(dllexport) int GetThreadCount();

/// <summary>
/// Gets the total number of handles across all processes.
/// </summary>
/// <returns>Handle count</returns>
__declspec(dllexport) int GetHandleCount();

/// <summary>
/// Gets system information string (Windows version, build number).
/// </summary>
/// <param name="buffer">Wide-character buffer to receive the info</param>
/// <param name="bufferSize">Size of buffer in characters (recommended: 512)</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool GetSystemInfoString(wchar_t* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// Hardware Temperature & Sensors (Placeholders)
// ---------------------------------------------------------------------------

/// <summary>
/// Gets motherboard temperature in Celsius.
/// NOTE: Placeholder. Requires WMI or hardware monitoring libraries.
/// </summary>
/// <returns>Temperature in Celsius, or -1.0 if not available</returns>
__declspec(dllexport) float GetMotherboardTemperature();

/// <summary>
/// Gets fan speed in RPM for a specific fan.
/// NOTE: Placeholder. Requires hardware monitoring libraries.
/// </summary>
/// <param name="fanIndex">Zero-based fan index</param>
/// <returns>Fan speed in RPM, or -1 if not available</returns>
__declspec(dllexport) int GetFanSpeed(int fanIndex);

// ---------------------------------------------------------------------------
// Utility Functions (Shell & UI)
// ---------------------------------------------------------------------------

/// <summary>
/// Renders a 256x256 icon for the specified file/folder path.
/// Returns pointer to static buffer containing raw BGRA pixel data.
/// </summary>
/// <param name="path">Wide-character path to file or folder</param>
/// <param name="bmpLength">Output: Size of returned buffer in bytes (256*256*4 = 262144)</param>
/// <returns>Pointer to static pixel buffer (BGRA, top-down), or nullptr on failure</returns>
__declspec(dllexport) BYTE* GetJumboIcon(const wchar_t* path, int* bmpLength);

/// <summary>
/// Simulates a keyboard key press (down + up).
/// </summary>
/// <param name="keyCode">Virtual key code (e.g., VK_SPACE, VK_RETURN)</param>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool SimulateKeypress(unsigned int keyCode);

/// <summary>
/// Gets the file path to the current user's account picture/avatar.
/// </summary>
/// <param name="username">Username (currently unused, uses current user)</param>
/// <param name="buffer">Byte buffer to receive ASCII path string</param>
/// <returns>true if avatar found, false otherwise</returns>
__declspec(dllexport) bool GetAvatarPath(const wchar_t* username, BYTE* buffer);

// ---------------------------------------------------------------------------
// Mouse Hook Functions
// ---------------------------------------------------------------------------

/// <summary>
/// Installs a low-level mouse hook to monitor mouse button events.
/// </summary>
/// <param name="hMod">Module handle (unused, kept for compatibility)</param>
/// <returns>true if hook installed successfully, false otherwise</returns>
__declspec(dllexport) bool SetMouseHook(HINSTANCE hMod);

/// <summary>
/// Removes the mouse hook.
/// </summary>
/// <returns>true if successful, false otherwise</returns>
__declspec(dllexport) bool UnhookMouseHook();

/// <summary>
/// Checks if a mouse down event occurred (cleared on read).
/// </summary>
/// <returns>true if left mouse button was pressed since last call, false otherwise</returns>
__declspec(dllexport) bool GetMouseDown();

/// <summary>
/// Checks if a mouse up event occurred (cleared on read).
/// </summary>
/// <returns>true if left mouse button was released since last call, false otherwise</returns>
__declspec(dllexport) bool GetMouseUp();

#ifdef __cplusplus
}
#endif
