# KDP Compatible Unsigned Driver Loader (Static Library)
Kernel unsigned driver loader static library,  KDP compatible,  leveraging gdrv.sys's write primitive.

Provides a C API for loading unsigned drivers programmatically from your own projects.

Tested on Windows 10 21H2 and 22H2

# Usage

Link against `gdrv-loader.lib` and include `gdrv_loader.h`. You will also need to link: `ntdllp_7.lib`, `kernel32.lib`, `shlwapi.lib`.

**Load a driver:**
```cpp
#include "gdrv_loader.h"

// Optional: receive status/debug messages
void LogCallback(const wchar_t* msg) { wprintf(L"%s", msg); }

int main() {
    GdrvSetLogCallback(LogCallback);

    // Load an unsigned driver using gdrv.sys as the vulnerable loader
    NTSTATUS status = GdrvLoadDriver(L"gdrv.sys", L"mydriver.sys", FALSE);
    if (status != 0)
        wprintf(L"Load failed: %08X\n", status);

    return 0;
}
```

**Unload a driver:**
```cpp
NTSTATUS status = GdrvUnloadDriver(L"mydriver.sys", FALSE);
```

**Query system information:**
```cpp
GDRV_SYSTEM_INFO info;
GdrvQuerySystemInfo(&info);

if (info.CodeIntegrityInfoValid)
    printf("CI Options: 0x%04X\n", info.CodeIntegrityOptions);
if (info.BootInfoValid)
    printf("Firmware: %s\n", info.FirmwareType == 2 ? "UEFI" : "BIOS");
```

# How it works 
Driver Signature Enforcement is implemented within CI.dll. Based on Reverse Engineering of the signature validation process we know nt!SeValidateImageHeader calls CI!CiValidateImageHeader.  
the return status from CiValidateImageHeader determines whether the signature is valid or not.   
Based on Reverse Engineering of nt!SeValidateImageHeader we understand it uses an array -  SeCiCallbacks to retrieve the address of CiValidateImageHeader before calling it.  
SeCiCallbacks is initialized by CiInitialize.  to be precise,  a pointer to nt!SeCiCallbacks is passed to CiInitialize as an argument allowing us to map ntoskrnl.exe to usermode and perform the following:   
sig scan for the lea instruction prior to the CiIntialize call.  
calculate  the address of SeCiCallbacks in usermode  
calculate the offset from the base of ntoskrnl in usermode  
add the same offset to the base of ntoskrnl.exe in kernelmode.  
once we have the address of SeCiCallbacks in kernel, all we need to do is to add a static offset to CiValidateImageHeader's entry in the array.  
leverage the write primitive to replace the address of CiValidateImageHeader with the address of ZwFlushInstructionCache, or any function that will always return NTSTATUS SUCCESS with the same prototype of CiValidateImageHeader. 
***************************
# API Reference

| Function | Description |
|---|---|
| `GdrvSetLogCallback(callback)` | Set an optional callback to receive log messages |
| `GdrvLoadDriver(loaderPath, driverPath, hidden)` | Load an unsigned driver via the gdrv.sys exploit |
| `GdrvUnloadDriver(driverPath, hidden)` | Unload a previously loaded driver |
| `GdrvQuerySystemInfo(info)` | Query boot, code integrity, and debugger info |

All functions return `NTSTATUS`. The caller must run as administrator.

# Notes
- in case loading gdrv.sys fails, its likely due to Microsoft's driver blocklist/cert expired,  just use an alternative vulnerable driver , there are plenty of them.
- you can also disable the driver blocklist via the following command :  reg add HKLM\SYSTEM\CurrentControlSet\Control\CI\Config /v "VulnerableDriverBlocklistEnable" /t REG_DWORD /d 0 /f      
- whilst the implemented technique does not require a read primitive , we do use the read primitive to restore the original CiValidateImageHeader after the unsigned driver is loaded.   
you can modify the code to not use the read primitive and it will work just fine since  SeCiCallbacks is not PatchGuard protected (EDIT : protected in Windows 11 23H2)

- built on top of the core of gdrv-loader
- requires x64, Windows 7+
