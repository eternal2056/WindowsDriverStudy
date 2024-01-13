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
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnload\n"));
}

VOID GetProcess() {
	PEPROCESS Process = NULL;
	PsLookupProcessByProcessId(3944, &Process);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process %p \n", Process));
	UCHAR* ImageName = PsGetProcessImageFileName(Process);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process %p \n", ImageName));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process %s \n", ImageName));
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
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IrpCommon called\n"));
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
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IRP_MJ_CREATE called\n"));
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
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IRP_MJ_CLOSE called\n"));
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);



	return STATUS_SUCCESS;
	GetProcess();

}
NTSTATUS ReadFileDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	PVOID buffer = NULL;
	ULONG bytesRead = 0;

	// ---------------------------------- ReadFile(hDevice, buffer, bufferSize, &bytesRead, NULL) ----------------------------------
	// irpStack->Parameters.Read.Length 就是 R0 传过来的大小 bufferSize
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ReadFileDevice -> irpStack->Parameters.Read.Length: %d\n", irpStack->Parameters.Read.Length));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ReadFileDevice -> irpStack->Parameters.Write.Length: %d\n", irpStack->Parameters.Write.Length));
	// Irp->IoStatus.Information 就是 R0 要读过去的大小 bytesRead
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ReadFileDevice -> Irp->IoStatus.Information: %d\n", Irp->IoStatus.Information));

	// 获取I/O请求的缓冲区和长度
	if (irpStack->Parameters.Read.Length > 0) {
		buffer = ExAllocatePoolWithTag(NonPagedPool, irpStack->Parameters.Read.Length, "MyTag");
		if (buffer == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto End;
		}
	}

	// 在这里执行实际的读取操作，可能涉及到设备硬件或者其他数据源
	// 为了示例，这里只是将一些虚拟数据复制到用户提供的缓冲区
	// 实际中，你需要根据你的设备和场景进行相应的处理
	RtlCopyMemory(buffer, "Hello, this is some data.", irpStack->Parameters.Read.Length);
	bytesRead = irpStack->Parameters.Read.Length;

	// 将数据复制到I/O请求的缓冲区
	if (buffer != NULL) {
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = bytesRead;
		RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, buffer, bytesRead);
		ExFreePoolWithTag(buffer, "MyTag");
	}
	else {
		// 如果没有缓冲区，可能是个空读取请求
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
	}

End:
	// 完成请求
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}
NTSTATUS
WriteFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
	ULONG bytesWrite = 0;

	// ---------------------------------- ReadFile(hDevice, buffer, bufferSize, &bytesRead, NULL) ----------------------------------
	// irpStack->Parameters.Read.Length 就是 R0 传过来的大小 bufferSize
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WriteFileDevice -> irpStack->Parameters.Read.Length: %d\n", irpStack->Parameters.Read.Length));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WriteFileDevice -> irpStack->Parameters.Write.Length: %d\n", irpStack->Parameters.Write.Length));
	// Irp->IoStatus.Information 就是 R0 要读过去的大小 bytesRead
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WriteFileDevice -> Irp->IoStatus.Information: %d\n", Irp->IoStatus.Information));

	RtlCopyMemory(buffer, "Hello, this is some data.", irpStack->Parameters.Write.Length);
	bytesWrite = irpStack->Parameters.Write.Length;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Received data to write: %.*s\n", bytesWrite, buffer));

	// 设置I/O请求的状态和信息
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = bytesWrite;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;

}
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverEntry\n"));
	//DbgBreakPoint();

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
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create device (0x%X)\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
	if (NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Symbolic link created successfully.\n"));
	}
	else {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create symbolic link. Status: 0x%X\n", status));
		return status;
	}

	deviceObject->Flags |= DO_BUFFERED_IO;

	status = STATUS_SUCCESS;

	return status;
}