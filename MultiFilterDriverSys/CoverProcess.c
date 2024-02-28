#include "CoverProcess.h"

typedef __int64(__fastcall* FChangeWindowTreeProtection)(void* a1, int a2);
FChangeWindowTreeProtection g_ChangeWindowTreeProtection = 0;

typedef __int64(__fastcall* FValidateHwnd)(__int64 a1);
FValidateHwnd g_ValidateHwnd = 0;

// 获取模块基址
BOOLEAN get_module_base_address(const char* name, unsigned long long* addr, unsigned long* size)
{
	unsigned long need_size = 0;
	ZwQuerySystemInformation(11, &need_size, 0, &need_size);
	if (need_size == 0) return FALSE;

	const unsigned long tag = 'VMON';
	PSYSTEM_MODULE_INFORMATION sys_mods = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, need_size, tag);
	if (sys_mods == 0) return FALSE;

	NTSTATUS status = ZwQuerySystemInformation(11, sys_mods, need_size, 0);
	if (!NT_SUCCESS(status))
	{
		ExFreePoolWithTag(sys_mods, tag);
		return FALSE;
	}

	for (unsigned long long i = 0; i < sys_mods->ulModuleCount; i++)
	{
		PSYSTEM_MODULE mod = &sys_mods->Modules[i];
		if (strstr(mod->ImageName, name))
		{
			*addr = (unsigned long long)mod->Base;
			*size = (unsigned long)mod->Size;
			break;
		}
	}

	ExFreePoolWithTag(sys_mods, tag);
	return TRUE;
}

// 模式匹配
BOOLEAN pattern_check(const char* data, const char* pattern, const char* mask)
{
	size_t len = strlen(mask);

	for (size_t i = 0; i < len; i++)
	{
		if (data[i] == pattern[i] || mask[i] == '?')
			continue;
		else
			return FALSE;
	}

	return TRUE;
}
unsigned long long find_pattern(unsigned long long addr, unsigned long size, const char* pattern, const char* mask)
{
	size -= (unsigned long)strlen(mask);

	for (unsigned long i = 0; i < size; i++)
	{
		if (pattern_check((const char*)addr + i, pattern, mask))
			return addr + i;
	}

	return 0;
}
unsigned long long find_pattern_image(unsigned long long addr, const char* pattern, const char* mask)
{
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)addr;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	PIMAGE_NT_HEADERS64 nt = (PIMAGE_NT_HEADERS64)(addr + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE)
		return 0;

	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);

	for (unsigned short i = 0; i < nt->FileHeader.NumberOfSections; i++)
	{
		PIMAGE_SECTION_HEADER p = &section[i];

		if (strstr((const char*)p->Name, ".text") || 'EGAP' == *(int*)(p->Name))
		{
			unsigned long long res = find_pattern(addr + p->VirtualAddress, p->Misc.VirtualSize, pattern, mask);
			if (res) return res;
		}
	}

	return 0;
}

// 获取导出函数
void* get_system_base_export(const char* module_name, const char* routine_name)
{
	unsigned long long win32kbase_address = 0;
	unsigned long win32kbase_length = 0;
	get_module_base_address(module_name, &win32kbase_address, &win32kbase_length);
	DbgPrintEx(0, 0, "[+] %s base address is 0x%llX \n", module_name, win32kbase_address);
	//if (MmIsAddressValid((void*)win32kbase_address) == FALSE) return 0;

	return RtlFindExportedRoutineByName((void*)win32kbase_address, routine_name);
}

// 初始化
BOOLEAN initialize()
{
	unsigned long long win32kfull_address = 0;
	unsigned long win32kfull_length = 0;
	get_module_base_address("win32kfull.sys", &win32kfull_address, &win32kfull_length);
	DbgPrintEx(0, 0, "[+] win32kfull base address is 0x%llX \n", win32kfull_address);
	//KdBreakPoint();
	//if (MmIsAddressValid((void*)win32kfull_address) == FALSE) return false;

	/*
	call    ?ChangeWindowTreeProtection@@YAHPEAUtagWND@@H@Z ; ChangeWindowTreeProtection(tagWND *,int)
	mov     esi, eax
	test    eax, eax
	jnz     short loc_1C0245002
	*/
	unsigned long long address = find_pattern_image(win32kfull_address,
		"\xE8\x00\x00\x00\x00\x8B\xF0\x85\xC0\x75\x00\x44\x8B\x44",
		"x????xxxxx?xxx");
	DbgPrintEx(0, 0, "[+] pattern address is 0x%llX \n", address);
	if (address == 0) return FALSE;

	// 5=汇编指令长度
	// 1=偏移
	g_ChangeWindowTreeProtection = (FChangeWindowTreeProtection)((char*)(address)+5 + *(int*)((char*)(address)+1));
	DbgPrintEx(0, 0, "[+] ChangeWindowTreeProtection address is 0x%p \n", g_ChangeWindowTreeProtection);
	if (MmIsAddressValid(g_ChangeWindowTreeProtection) == FALSE) return FALSE;

	g_ValidateHwnd = (FValidateHwnd)get_system_base_export("win32kbase.sys", "ValidateHwnd");
	DbgPrintEx(0, 0, "[+] ValidateHwnd address is 0x%p \n", g_ValidateHwnd);
	if (MmIsAddressValid(g_ValidateHwnd) == FALSE) return FALSE;

	return TRUE;
}

// 修改窗口状态
__int64 change_window_attributes(__int64 handler, int attributes)
{
	if (MmIsAddressValid(g_ChangeWindowTreeProtection) == FALSE) return 0;
	if (MmIsAddressValid(g_ValidateHwnd) == FALSE) return 0;

	void* wnd_ptr = (void*)g_ValidateHwnd(handler);
	if (MmIsAddressValid(wnd_ptr) == FALSE) return 0;

	return g_ChangeWindowTreeProtection(wnd_ptr, attributes);
}

VOID CoverProcessDriverUnload(DRIVER_OBJECT* DriverObject) {
	UNICODE_STRING symbolicLinkName;

	RtlInitUnicodeString(&symbolicLinkName, KILLRULE_DOSDEVICE_NAME); // 替换为实际链接名称
	IoDeleteSymbolicLink(&symbolicLinkName);
	KdBreakPoint();
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
	PMyMessage64 info = NULL;
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
	case HIDE_WINDOW:
		info = (MyMessage64*)inbuf;
		if (ctlcode == HIDE_WINDOW && MmIsAddressValid(info))
		{
			// 初始化
			static BOOLEAN init = FALSE;
			if (!init) init = initialize();

			info->window_result = change_window_attributes(info->window_handle, info->window_attributes);
		}
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