#include "global.h"
#include "gdrv_loader.h"

NTSTATUS
QuerySystemInformation(
	_Out_ GDRV_SYSTEM_INFO* Info
	)
{
	RtlZeroMemory(Info, sizeof(*Info));

	NTSTATUS Status;

	// Boot environment
	SYSTEM_BOOT_ENVIRONMENT_INFORMATION BootInfo = { 0 };
	Status = NtQuerySystemInformation(SystemBootEnvironmentInformation,
										&BootInfo,
										sizeof(BootInfo),
										nullptr);
	if (NT_SUCCESS(Status))
	{
		Info->BootIdentifier = BootInfo.BootIdentifier;
		Info->FirmwareType = BootInfo.FirmwareType;
		Info->BootFlags = BootInfo.BootFlags;
		Info->BootInfoValid = TRUE;
	}

	// Kernel module
	ULONG Size = 0;
	Status = NtQuerySystemInformation(SystemModuleInformation,
										nullptr,
										0,
										&Size);
	if (Status == STATUS_INFO_LENGTH_MISMATCH)
	{
		PRTL_PROCESS_MODULES ModuleInfo = static_cast<PRTL_PROCESS_MODULES>(
			RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, 2 * Size));
		if (ModuleInfo != nullptr)
		{
			Status = NtQuerySystemInformation(SystemModuleInformation,
											ModuleInfo,
											2 * Size,
											nullptr);
			if (NT_SUCCESS(Status) && ModuleInfo->NumberOfModules > 0)
			{
				RTL_PROCESS_MODULE_INFORMATION Ntoskrnl = ModuleInfo->Modules[0];
				const char* fileName = reinterpret_cast<const char*>(Ntoskrnl.FullPathName + Ntoskrnl.OffsetToFileName);
				const char* fullPath = reinterpret_cast<const char*>(Ntoskrnl.FullPathName);

				strncpy(Info->KernelFileName, fileName, sizeof(Info->KernelFileName) - 1);
				strncpy(Info->KernelFullPath, fullPath, sizeof(Info->KernelFullPath) - 1);
				Info->KernelInfoValid = TRUE;
			}
			RtlFreeHeap(RtlProcessHeap(), 0, ModuleInfo);
		}
	}

	// Code integrity
	SYSTEM_CODEINTEGRITY_INFORMATION CodeIntegrityInfo = { sizeof(SYSTEM_CODEINTEGRITY_INFORMATION) };
	Status = NtQuerySystemInformation(SystemCodeIntegrityInformation,
										&CodeIntegrityInfo,
										sizeof(CodeIntegrityInfo),
										nullptr);
	if (NT_SUCCESS(Status))
	{
		Info->CodeIntegrityOptions = CodeIntegrityInfo.CodeIntegrityOptions;
		Info->CodeIntegrityInfoValid = TRUE;
	}

	// Kernel debugger
	SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInfo = { 0 };
	Status = NtQuerySystemInformation(SystemKernelDebuggerInformation,
										&KernelDebuggerInfo,
										sizeof(KernelDebuggerInfo),
										nullptr);
	if (NT_SUCCESS(Status))
	{
		Info->KernelDebuggerEnabled = KernelDebuggerInfo.KernelDebuggerEnabled;
		Info->KernelDebuggerNotPresent = KernelDebuggerInfo.KernelDebuggerNotPresent;
		Info->KernelDebuggerInfoValid = TRUE;
	}

	// Extended debugger info (Windows 8.1+)
	if ((RtlNtMajorVersion() >= 6 && RtlNtMinorVersion() >= 3) || RtlNtMajorVersion() > 6)
	{
		SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX KernelDebuggerInfoEx = { 0 };
		Status = NtQuerySystemInformation(SystemKernelDebuggerInformationEx,
											&KernelDebuggerInfoEx,
											sizeof(KernelDebuggerInfoEx),
											nullptr);
		if (NT_SUCCESS(Status))
		{
			Info->DebuggerAllowed = KernelDebuggerInfoEx.DebuggerAllowed;
			Info->DebuggerEnabled = KernelDebuggerInfoEx.DebuggerEnabled;
			Info->DebuggerPresent = KernelDebuggerInfoEx.DebuggerPresent;
			Info->KernelDebuggerInfoExValid = TRUE;
		}
	}

	// SharedUserData
	Info->KdDebuggerEnabledByte = SharedUserData->KdDebuggerEnabled;

	// Windows 10+ fields
	if (RtlNtMajorVersion() > 6)
	{
		UCHAR KernelDebuggerFlags = 0;
		Status = NtQuerySystemInformation(SystemKernelDebuggerFlags,
											&KernelDebuggerFlags,
											sizeof(KernelDebuggerFlags),
											nullptr);
		if (NT_SUCCESS(Status))
		{
			Info->KernelDebuggerFlags = KernelDebuggerFlags;
			Info->KernelDebuggerFlagsValid = TRUE;
		}

		SYSTEM_CODEINTEGRITYPOLICY_INFORMATION CodeIntegrityPolicyInfo = { 0 };
		Status = NtQuerySystemInformation(SystemCodeIntegrityPolicyInformation,
											&CodeIntegrityPolicyInfo,
											sizeof(CodeIntegrityPolicyInfo),
											nullptr);
		if (NT_SUCCESS(Status))
		{
			Info->CodeIntegrityPolicyOptions = CodeIntegrityPolicyInfo.Options;
			Info->CodeIntegrityPolicyHVCIOptions = CodeIntegrityPolicyInfo.HVCIOptions;
			Info->CodeIntegrityPolicyInfoValid = TRUE;
		}

		BOOLEAN KernelDebuggingAllowed = FALSE;
		Status = NtQuerySystemInformation(SystemKernelDebuggingAllowed,
										&KernelDebuggingAllowed,
										0,
										nullptr);
		if (Status == STATUS_SECUREBOOT_NOT_ENABLED)
		{
			Info->SecureBootEnabled = FALSE;
			Info->SecureBootInfoValid = TRUE;
		}
		else if (NT_SUCCESS(Status))
		{
			Info->SecureBootEnabled = TRUE;
			Info->SecureBootInfoValid = TRUE;
		}
	}

	return STATUS_SUCCESS;
}
