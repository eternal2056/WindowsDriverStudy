#include <ntifs.h>

NTKERNELAPI
NTSTATUS PsLookupProcessByProcessId(
	__in HANDLE ProcessId,
	__deref_out PEPROCESS* Process
);
NTKERNELAPI
UCHAR*
PsGetProcessImageFileName(
	__in PEPROCESS Process
);
VOID DriverUnload(DRIVER_OBJECT* DriverObject) {
	DbgPrintEx(77, 0, "DriverUnload\n");
}

VOID GetProcess() {
	PEPROCESS Process = NULL;
	PsLookupProcessByProcessId(3944, &Process);
	DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "Process %p \n", Process);
	UCHAR* ImageName = PsGetProcessImageFileName(Process);
	DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "Process %p \n", ImageName);
	DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "Process %s \n", ImageName);
	if (Process) {
		ObDereferenceObject(Process);
	}
}

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	DriverObject->DriverUnload = DriverUnload;
	DbgPrintEx(77, 0, "DriverEntry\n");
	DbgBreakPoint();
	GetProcess();
	NTSTATUS status;
	status = STATUS_SUCCESS;

	return status;
}