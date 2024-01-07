#include <ntifs.h>


VOID DriverUnload(DRIVER_OBJECT* DriverObject) {
	DbgPrintEx(77, 0, "DriverUnload\n");
}

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	DriverObject->DriverUnload = DriverUnload;
	DbgPrintEx(77, 0, "DriverEntry\n");
	NTSTATUS status;
	status = STATUS_SUCCESS;

	return status;
}