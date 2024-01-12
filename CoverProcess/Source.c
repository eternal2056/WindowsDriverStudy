#include <ntifs.h>
#include <ntstrsafe.h>

#define MY_DEVICE_NAME L"\\Device\\MyCoverProcess"
#define MY_SYMBOLIC_LINK_NAME L"\\DosDevices\\MyCoverProcessLink"

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
	UNICODE_STRING symbolicLinkName;

	RtlInitUnicodeString(&symbolicLinkName, MY_SYMBOLIC_LINK_NAME); // 替换为实际链接名称
	IoDeleteSymbolicLink(&symbolicLinkName);
	IoDeleteDevice(DriverObject->DeviceObject);
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

NTSTATUS
IrpCommon(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	DbgPrintEx(77, 0, "IrpCommon called\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS
CreateFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	DbgPrintEx(77, 0, "IRP_MJ_CREATE called\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);



	return STATUS_SUCCESS;
	GetProcess();

}
NTSTATUS
CloseFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	DbgPrintEx(77, 0, "IRP_MJ_CLOSE called\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);



	return STATUS_SUCCESS;
	GetProcess();

}
NTSTATUS
ReadFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	DbgPrintEx(77, 0, "IRP_MJ_READ called\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);



	return STATUS_SUCCESS;
	GetProcess();

}
NTSTATUS
WriteFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	DbgPrintEx(77, 0, "IRP_MJ_WRITE called\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);



	return STATUS_SUCCESS;
	GetProcess();

}
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	DbgPrintEx(77, 0, "DriverEntry\n");
	DbgBreakPoint();

	UNREFERENCED_PARAMETER(RegistryPath);

	PDEVICE_OBJECT deviceObject;
	UNICODE_STRING deviceName;
	UNICODE_STRING symbolicLinkName;

	RtlInitUnicodeString(&deviceName, MY_DEVICE_NAME);
	RtlInitUnicodeString(&symbolicLinkName, MY_SYMBOLIC_LINK_NAME); // 替换为实际链接名称

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = IrpCommon;
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateFileDevice;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseFileDevice;
	DriverObject->MajorFunction[IRP_MJ_READ] = ReadFileDevice;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = WriteFileDevice;

	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&deviceObject);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(77, 0, "Failed to create device (0x%X)\n", status);
		return status;
	}

	status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
	if (NT_SUCCESS(status))
	{
		DbgPrintEx(77, 0, "Symbolic link created successfully.\n");
	}
	else {
		DbgPrintEx(77, 0, "Failed to create symbolic link. Status: 0x%X\n", status);
		return status;
	}

	deviceObject->Flags |= DO_BUFFERED_IO;

	status = STATUS_SUCCESS;

	return status;
}