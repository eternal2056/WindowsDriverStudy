#include "CoverProcess.h"

VOID CoverProcessDriverUnload(DRIVER_OBJECT* DriverObject) {
	UNICODE_STRING symbolicLinkName;

	RtlInitUnicodeString(&symbolicLinkName, KILLRULE_DOSDEVICE_NAME); // 替换为实际链接名称
	IoDeleteSymbolicLink(&symbolicLinkName);
	DbgBreakPoint();
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnload\n"));
}

VOID GetProcess(ULONG processId) {
	PEPROCESS Process = NULL;
	PsLookupProcessByProcessId(processId, &Process);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process %p \n", Process));
	UCHAR* ImageName = PsGetProcessImageFileName(Process);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process %p \n", ImageName));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process %s \n", ImageName));
	if (Process) {
		ObDereferenceObject(Process);
	}
}
// 获取(ActiveProcessLinks)进程链表的相对(EPROCESS)偏移
ULONG GetProcessOffset()
{
	return 0x448;
	if (SharedUserData->NtMajorVersion == 5) // 如果位 Windows Xp
	{
		return 0x88;
	}
	else if ((SharedUserData->NtMajorVersion == 6) &&
		(SharedUserData->NtMinorVersion == 1)) // 如果为Windows7
	{
		if (sizeof(PVOID) == 4) // 如果为 32 位Windows7
		{
			return 0x0b8;
		}
		else // 如果为 64 位Windows7
		{
			return 0x188;
		}
	}
	return 0;
}
VOID HideProcess(HANDLE hProcessId)
{
	PLIST_ENTRY BmpList = { 0 };
	PEPROCESS Process;
	ULONG offset;
	NTSTATUS status = PsLookupProcessByProcessId(hProcessId, &Process);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "查找进程失败\n"));
		return;
	}
	BmpList = (PLIST_ENTRY)(((PUCHAR)Process + 0x448)); // WINDOWS 10 X64 ActiveProcessLinks
	//隐藏进程
	BmpList->Flink->Blink = BmpList->Blink;
	BmpList->Blink->Flink = BmpList->Flink;

	// --------------------------------------------------- 以下为失败品 | 谨记 ------------------------------------------ //

	//offset = GetProcessOffset();
	//if (offset == 0) { DbgPrint("offset == 0\n"); return; }
	//if (MiProcessLoaderEntry != NULL)
	//{
	//	// 将进程从进程链表中剔除, 达到隐藏进程的效果
	//	MiProcessLoaderEntry((PCHAR)Process + offset/*LDR_DATA_TABLE_ENTRY驱动结构体*/, // 会蓝屏
	//		FALSE/*FALSE为从驱动链表中删除，TRUE为插入*/);
	//	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "隐藏进程成功\n"));
	//}
	//else
	//{
	//	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "隐藏进程失败\n"));
	//}
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

}
NTSTATUS ReadFileDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IRP_MJ_READ called\n"));
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
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IRP_MJ_WRITE called\n"));
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

NTSTATUS DuplicateInputBuffer(IN PIRP irp, PVOID inbuf)
{
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpstack;
	PVOID inbuf_dup = NULL;
	PVOID outbuf = NULL;
	ULONG inlen = 0;
	irpstack = IoGetCurrentIrpStackLocation(irp);
	inlen = irpstack->Parameters.DeviceIoControl.InputBufferLength - 4;
	if (inbuf && inlen) {
		inbuf_dup = ExAllocatePoolWithTag(NonPagedPool, inlen, KILLRULE_POOLTAG);
		if (!inbuf_dup) return STATUS_MEMORY_NOT_ALLOCATED;
		RtlCopyMemory(inbuf_dup, inbuf, inlen);
		inbuf = inbuf_dup;
	}
	return status;
}

NTSTATUS MainDispatcher(PDEVICE_OBJECT devobj, PIRP irp)
{
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IRP_MJ_DEVICE_CONTROL called\n"));
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION	irpstack;
	PVOID inbuf_dup = NULL;
	PVOID inbuf = NULL;
	PVOID outbuf = NULL;
	ULONG inlen = 0;
	ULONG outlen = 0;
	ULONG ctlcode = 0;
	PROCESS_MY* op = 0;

	irpstack = IoGetCurrentIrpStackLocation(irp);
	ctlcode = irpstack->Parameters.DeviceIoControl.IoControlCode;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MainDispatcher -> irpstack->Parameters.DeviceIoControl.IoControlCode: %d\n", ctlcode));

	// [TODO] try except ProbeForRead/Write
	inlen = irpstack->Parameters.DeviceIoControl.InputBufferLength;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MainDispatcher -> irpstack->Parameters.DeviceIoControl.InputBufferLength: %d\n", inlen));
	if (inlen < 4) return STATUS_INVALID_PARAMETER;
	inbuf = irp->AssociatedIrp.SystemBuffer;
	if (!inbuf) return STATUS_INVALID_PARAMETER;
	op = (PROCESS_MY*)inbuf;

	//inlen = inlen - 4;
	//inbuf = (UCHAR*)inbuf + 4;

	//status = DuplicateInputBuffer(irp, inbuf);
	//if (!NT_SUCCESS(status)) return status;

	//outbuf = irp->AssociatedIrp.SystemBuffer;
	//outlen = irpstack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (ctlcode) {
	case IOCTL_KILLRULE_HEARTBEAT:
		status = STATUS_SUCCESS;
		break;
	case IOCTL_KILLRULE_DRIVER:
		//status = DriverDispatcher(op, devobj, irp);
		break;
	case IOCTL_KILLRULE_NOTIFY:
		//status = NotifyDispatcher(op, devobj, irp);
		break;
	case IOCTL_KILLRULE_MEMORY:
		//status = MemoryDispatcher(op, devobj, inbuf, inlen, outbuf, outlen, irp);
		break;
	case IOCTL_KILLRULE_HOTKEY:
		//status = HotkeyDispatcher(op, devobj, irp);
		break;
	case IOCTL_KILLRULE_STORAGE:
		//status = StorageDispatcher(op, devobj, irp);
		break;
	case IOCTL_KILLRULE_OBJECT:
		//status = ObjectDispatcher(op, devobj, inbuf, inlen, outbuf, outlen, irp);
		break;
	case IOCTL_KILLRULE_PROCESS:
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTL_KILLRULE_PROCESS called\n"));
		//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MainDispatcher -> op->Value1: %d\n", op->Value1));

		//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MainDispatcher -> op->Value2: %d\n", op->Value2));
		//KdBreakPoint();
		//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MainDispatcher -> op->Value2: %d\n", op->MyPoint->Value1));
		//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MainDispatcher -> op->Value2: %d\n", op->MyPoint->Value2));
		//status = ProcessDispatcher(op, devobj, inbuf, inlen, outbuf, outlen, irp);
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MainDispatcher -> op->ProcessId: %d\n", op->ProcessId));
		ULONG processId = op->ProcessId;
		GetProcess(processId);
		HideProcess(processId);
		break;
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS CoverProcessDriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	//MiProcessLoaderEntry = (pMiProcessLoaderEntry)0xfffff80534b88ee4; // 这是用 dp 来定位的
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CoverProcessDriverEntry\n"));
	//KdBreakPoint();

	UNREFERENCED_PARAMETER(RegistryPath);

	PDEVICE_OBJECT deviceObject;
	UNICODE_STRING deviceName;
	UNICODE_STRING symbolicLinkName;

	RtlInitUnicodeString(&deviceName, KILLRULE_NTDEVICE_NAME);
	RtlInitUnicodeString(&symbolicLinkName, KILLRULE_DOSDEVICE_NAME); // 替换为实际链接名称

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = IrpCommon;
	}

	DriverObject->DriverUnload = CoverProcessDriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateFileDevice;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseFileDevice;
	DriverObject->MajorFunction[IRP_MJ_READ] = ReadFileDevice;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = WriteFileDevice;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MainDispatcher;

	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&deviceObject);
	UNICODE_STRING DeviceName = GetDeviceObjectName(deviceObject);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CoverProcessDriverEntry %wZ\n", &DeviceName));
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