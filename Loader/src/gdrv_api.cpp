#include "global.h"
#include "gdrv_loader.h"

// Global log callback instance
GDRV_LOG_CALLBACK_INTERNAL g_LogCallback = nullptr;

// Internal declarations - WindLoadDriver/WindUnloadDriver have C linkage via global.h
extern "C" {
NTSTATUS WindLoadDriver(PWCHAR LoaderName, PWCHAR DriverName, BOOLEAN Hidden);
NTSTATUS WindUnloadDriver(PWCHAR DriverName, BOOLEAN Hidden);
}
// QuerySystemInformation has C++ linkage (defined in sys.cpp)
NTSTATUS QuerySystemInformation(GDRV_SYSTEM_INFO* Info);

void GdrvSetLogCallback(GDRV_LOG_CALLBACK Callback)
{
	g_LogCallback = reinterpret_cast<GDRV_LOG_CALLBACK_INTERNAL>(Callback);
}

NTSTATUS GdrvLoadDriver(
	const wchar_t* LoaderDriverPath,
	const wchar_t* TargetDriverPath,
	BOOLEAN Hidden
)
{
	return WindLoadDriver(
		const_cast<PWCHAR>(LoaderDriverPath),
		const_cast<PWCHAR>(TargetDriverPath),
		Hidden
	);
}

NTSTATUS GdrvUnloadDriver(
	const wchar_t* TargetDriverPath,
	BOOLEAN Hidden
)
{
	return WindUnloadDriver(
		const_cast<PWCHAR>(TargetDriverPath),
		Hidden
	);
}

NTSTATUS GdrvQuerySystemInfo(GDRV_SYSTEM_INFO* Info)
{
	return QuerySystemInformation(Info);
}
