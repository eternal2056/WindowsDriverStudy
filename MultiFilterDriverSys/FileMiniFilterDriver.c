#include "FileMiniFilterDriver.h"
#include "Rule.h"
#include "Helper.h"
#include "ExcludeList.h"
//  Global variables
#define FSFILTER_ALLOC_TAG 'DHlF'
PFLT_FILTER gFilterHandle;
ULONG gTraceFlags = 0;
BOOLEAN g_deviceInitedMiniFilter = FALSE;
PDEVICE_OBJECT g_deviceObjectMiniFilter = NULL;

PFLT_FILTER gFilterHandle;
PFLT_PORT 	gServerPort;
PFLT_PORT 	gClientPort;

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((void)0))


NTSTATUS AddHiddenObject(PHid_HideObjectPacket Packet, USHORT Size, PULONGLONG ObjId)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING path;
	USHORT i, count;

	// Check can we access to the packet
	if (Size < sizeof(Hid_HideObjectPacket))
		return STATUS_INVALID_PARAMETER;

	// Check packet data size overflow
	if (Size < Packet->dataSize + sizeof(Hid_HideObjectPacket))
		return STATUS_INVALID_PARAMETER;

	// Unpack string to UNICODE_STRING

	path.Buffer = (LPWSTR)((PCHAR)Packet + sizeof(Hid_HideObjectPacket));
	path.MaximumLength = Size - sizeof(Hid_HideObjectPacket);

	// Just checking for zero-end string ends in the middle
	count = Packet->dataSize / sizeof(WCHAR);
	for (i = 0; i < count; i++)
		if (path.Buffer[i] == L'\0')
			break;

	path.Length = i * sizeof(WCHAR);

	// Perform the packet

	switch (Packet->objType)
	{
	case FsFileObject:
		status = AddHiddenFile(&path, ObjId);
		break;
	case FsDirObject:
		status = AddHiddenDir(&path, ObjId);
		break;
	default:
		LogWarning("Unsupported object type: %u", Packet->objType);
		return STATUS_INVALID_PARAMETER;
	}

	return status;
}

NTSTATUS RemoveHiddenObject(PHid_UnhideObjectPacket Packet, USHORT Size)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (Size != sizeof(Hid_UnhideObjectPacket))
		return STATUS_INVALID_PARAMETER;

	// Perform packet

	switch (Packet->objType)
	{
	case FsFileObject:
		status = RemoveHiddenFile(Packet->id);
		break;
	case FsDirObject:
		status = RemoveHiddenDir(Packet->id);
		break;
	default:
		LogWarning("Unsupported object type: %u", Packet->objType);
		return STATUS_INVALID_PARAMETER;
	}

	return status;
}

NTSTATUS RemoveAllHiddenObjects(PHid_UnhideAllObjectsPacket Packet, USHORT Size)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (Size != sizeof(Hid_UnhideAllObjectsPacket))
		return STATUS_INVALID_PARAMETER;

	// Perform packet

	switch (Packet->objType)
	{
	case FsFileObject:
		status = RemoveAllHiddenFiles();
		break;
	case FsDirObject:
		status = RemoveAllHiddenDirs();
		break;
	default:
		LogWarning("Unsupported object type: %u", Packet->objType);
		return STATUS_INVALID_PARAMETER;
	}

	return status;
}

_Function_class_(DRIVER_DISPATCH)
_Dispatch_type_(IRP_MJ_CREATE)
NTSTATUS IrpMiniFilterDeviceCreate(PDEVICE_OBJECT  DeviceObject, PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

_Function_class_(DRIVER_DISPATCH)
_Dispatch_type_(IRP_MJ_CLOSE)
NTSTATUS IrpMiniFilterDeviceClose(PDEVICE_OBJECT  DeviceObject, PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

_Function_class_(DRIVER_DISPATCH)
_Dispatch_type_(IRP_MJ_CLEANUP)
NTSTATUS IrpMiniFilterDeviceCleanup(PDEVICE_OBJECT  DeviceObject, PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

_Function_class_(DRIVER_DISPATCH)
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
NTSTATUS IrpMiniFilterDeviceControlHandler(PDEVICE_OBJECT  DeviceObject, PIRP  Irp)
{
	PIO_STACK_LOCATION irpStack;
	Hid_StatusPacket result;
	NTSTATUS status = STATUS_SUCCESS;
	PVOID inputBuffer, outputBuffer, outputData;
	ULONG ioctl, inputBufferSize, outputBufferSize, outputBufferMaxSize,
		outputDataMaxSize, outputDataSize;

	UNREFERENCED_PARAMETER(DeviceObject);

	// Get irp information

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	ioctl = irpStack->Parameters.DeviceIoControl.IoControlCode;

	inputBuffer = outputBuffer = Irp->AssociatedIrp.SystemBuffer;
	inputBufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outputBufferMaxSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	outputBufferSize = 0;
	outputDataSize = 0;
	outputDataMaxSize = 0;

	RtlZeroMemory(&result, sizeof(result));

	// Check output buffer size

	if (outputBufferMaxSize < sizeof(result))
	{
		status = STATUS_INVALID_PARAMETER;
		goto EndProc;
	}

	// Prepare additional buffer for output data 
	outputData = (PVOID)((UINT_PTR)outputBuffer + sizeof(result));
	outputDataMaxSize = outputBufferMaxSize - sizeof(result);

	// Important limitation:
	// Because both input (inputBuffer) and output data (outputData) are located in the same buffer there is a limitation for the output
	// buffer usage. When a ioctl handler is executing, it can use the input buffer only until first write to the output buffer, because
	// when you put data to the output buffer you can overwrite data in input buffer. Therefore if you gonna use both an input and output 
	// data in the same time you should make the copy of input data and work with it.
	switch (ioctl)
	{
		// Reg/Fs 
	case HID_IOCTL_ADD_HIDDEN_OBJECT:
		result.status = AddHiddenObject((PHid_HideObjectPacket)inputBuffer, (USHORT)inputBufferSize, &result.info.id);
		break;
	case HID_IOCTL_REMOVE_HIDDEN_OBJECT:
		result.status = RemoveHiddenObject((PHid_UnhideObjectPacket)inputBuffer, (USHORT)inputBufferSize);
		break;
	case HID_IOCTL_REMOVE_ALL_HIDDEN_OBJECTS:
		result.status = RemoveAllHiddenObjects((PHid_UnhideAllObjectsPacket)inputBuffer, (USHORT)inputBufferSize);
		break;
		// Other
	default:
		LogWarning("Unknown IOCTL code:%08x", ioctl);
		status = STATUS_INVALID_PARAMETER;
		goto EndProc;
	}

EndProc:

	// If additional output data has been presented
	if (NT_SUCCESS(status) && outputDataSize > 0)
	{
		if (outputDataSize > outputDataMaxSize)
		{
			LogWarning("An internal error, looks like a stack corruption!");
			outputDataSize = outputDataMaxSize;
			result.status = (ULONG)STATUS_PARTIAL_COPY;
		}

		result.dataSize = outputDataSize;
	}

	// Copy result to output buffer
	if (NT_SUCCESS(status))
	{
		outputBufferSize = sizeof(result) + outputDataSize;
		RtlCopyMemory(outputBuffer, &result, sizeof(result));
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = outputBufferSize;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS InitializeMiniFilterDevice(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(MINIFILTER_DEVICE_NAME);
	UNICODE_STRING dosDeviceName = RTL_CONSTANT_STRING(MINIFILTER_DOS_DEVICES_LINK_NAME);
	PDEVICE_OBJECT deviceObject = NULL;

	status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);
	if (!NT_SUCCESS(status))
	{
		LogError("Error, device creation failed with code:%08x", status);
		return status;
	}

	status = IoCreateSymbolicLink(&dosDeviceName, &deviceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(deviceObject);
		LogError("Error, symbolic link creation failed with code:%08x", status);
		return status;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpMiniFilterDeviceCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpMiniFilterDeviceClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = IrpMiniFilterDeviceCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpMiniFilterDeviceControlHandler;
	g_deviceObjectMiniFilter = deviceObject;
	g_deviceInitedMiniFilter = TRUE;

	LogTrace("Initialization is completed");
	return status;
}

NTSTATUS DestroyMiniFilterDevice()
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING dosDeviceName = RTL_CONSTANT_STRING(MINIFILTER_DOS_DEVICES_LINK_NAME);

	if (!g_deviceInitedMiniFilter)
		return STATUS_NOT_FOUND;

	status = IoDeleteSymbolicLink(&dosDeviceName);
	if (!NT_SUCCESS(status))
		LogWarning("Error, symbolic link deletion failed with code:%08x", status);

	IoDeleteDevice(g_deviceObjectMiniFilter);

	g_deviceInitedMiniFilter = FALSE;

	LogTrace("Deinitialization is completed");
	return status;
}


NTSTATUS MiniFilterDriverEntry(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status;
	PSECURITY_DESCRIPTOR sd;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uniString;		//for communication port name

	UNREFERENCED_PARAMETER(RegistryPath);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("NPminifilter!DriverEntry: Entered\n"));

	//
	//  Register with FltMgr to tell it our callback routines
	//

	status = InitializeMiniFilterDevice(DriverObject);
	if (!NT_SUCCESS(status))
		LogWarning("Error, can't create device");

	BOOLEAN bSucc = InitMinFilterRuleInfo();
	if (bSucc == FALSE)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	status = FltRegisterFilter(DriverObject,
		&FilterRegistration,
		&gFilterHandle);

	ASSERT(NT_SUCCESS(status));

	if (NT_SUCCESS(status)) {

		//
		//  Start filtering i/o
		//

		status = FltStartFiltering(gFilterHandle);

		if (!NT_SUCCESS(status)) {

			FltUnregisterFilter(gFilterHandle);
			UninitMinFilterRuleInfo();
		}
	}
	//Communication Port
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

	if (!NT_SUCCESS(status)) {
		goto final;
	}


	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

	if (!NT_SUCCESS(status)) {
		goto final;
	}

	RtlInitUnicodeString(&uniString, MINISPY_PORT_NAME);

	InitializeObjectAttributes(&oa,
		&uniString,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		sd);

	status = InitializeFSMiniFilter(DriverObject);

	status = FltCreateCommunicationPort(gFilterHandle,
		&gServerPort,
		&oa,
		NULL,
		NPMiniConnect,
		NPMiniDisconnect,
		NPMiniMessage,
		1);

	FltFreeSecurityDescriptor(sd);

	if (!NT_SUCCESS(status)) {
		goto final;
	}

	final :

	if (!NT_SUCCESS(status)) {

		if (NULL != gServerPort) {
			FltCloseCommunicationPort(gServerPort);
		}

		if (NULL != gFilterHandle) {
			FltUnregisterFilter(gFilterHandle);
		}
		UninitMinFilterRuleInfo();
		DestroyFSMiniFilter();
	}
	return status;
}

BOOLEAN NPUnicodeStringToChar(PUNICODE_STRING UniName, char Name[])
{
	ANSI_STRING	AnsiName;
	NTSTATUS	ntstatus;
	char* nameptr;

	__try {
		ntstatus = RtlUnicodeStringToAnsiString(&AnsiName, UniName, TRUE);

		if (AnsiName.Length < 260) {
			nameptr = (PCHAR)AnsiName.Buffer;
			//Convert into upper case and copy to buffer
			strcpy(Name, _strupr(nameptr));
			DbgPrint("NPUnicodeStringToChar : %s\n", Name);
		}
		RtlFreeAnsiString(&AnsiName);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		DbgPrint("NPUnicodeStringToChar EXCEPTION_EXECUTE_HANDLER\n");
		return FALSE;
	}
	return TRUE;
}

NTSTATUS
NPInstanceSetup(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_SETUP_FLAGS Flags,
	__in DEVICE_TYPE VolumeDeviceType,
	__in FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("NPminifilter!NPInstanceSetup: Entered\n"));

	return STATUS_SUCCESS;
}


NTSTATUS
NPInstanceQueryTeardown(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("NPminifilter!NPInstanceQueryTeardown: Entered\n"));

	return STATUS_SUCCESS;
}


VOID
NPInstanceTeardownStart(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("NPminifilter!NPInstanceTeardownStart: Entered\n"));
}


VOID
NPInstanceTeardownComplete(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("NPminifilter!NPInstanceTeardownComplete: Entered\n"));
}


/*************************************************************************
	MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
NPUnload(
	__in FLT_FILTER_UNLOAD_FLAGS Flags
)
{
	KdBreakPoint();
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("NPminifilter!NPUnload: Entered\n"));

	DestroyMiniFilterDevice();
	FltCloseCommunicationPort(gServerPort);
	FltUnregisterFilter(gFilterHandle);
	UninitMinFilterRuleInfo();
	DestroyFSMiniFilter();
	return STATUS_SUCCESS;
}


/*************************************************************************
	MiniFilter callback routines.
*************************************************************************/

FLT_PREOP_CALLBACK_STATUS
NPPreCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID* CompletionContext
)
{
	char FileName[260] = "X:";

	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PAGED_CODE();

	__try {
		status = FltGetFileNameInformation(Data,
			FLT_FILE_NAME_NORMALIZED |
			FLT_FILE_NAME_QUERY_DEFAULT,
			&nameInfo);
		if (NT_SUCCESS(status)) {
			//判斷是否阻擋
			FltParseFileNameInformation(nameInfo);
			if (NPUnicodeStringToChar(&nameInfo->Name, FileName)) {
				//KdPrintEx((77, 0, "%s %s %d IOCTL_WFP_SAMPLE_ADD_RULE %s\n", __FILE__, __FUNCTION__, __LINE__, FileName));
				if (IsHitMinifilterRuleFileName(FileName)) {
					Data->IoStatus.Status = STATUS_ACCESS_DENIED;
					Data->IoStatus.Information = 0;
					FltReleaseFileNameInformation(nameInfo);
					return FLT_PREOP_COMPLETE;
				}
			}
			//release resource
			FltReleaseFileNameInformation(nameInfo);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		DbgPrint("NPPreCreate EXCEPTION_EXECUTE_HANDLER\n");
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
NPPostCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
)
{
	FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	//
	//  If this create was failing anyway, don't bother scanning now.
	//

	if (!NT_SUCCESS(Data->IoStatus.Status) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	//
	//  Check if we are interested in this file.
	//

	status = FltGetFileNameInformation(Data,
		FLT_FILE_NAME_NORMALIZED |
		FLT_FILE_NAME_QUERY_DEFAULT,
		&nameInfo);

	if (!NT_SUCCESS(status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	return returnStatus;
}

//與user application Conect
NTSTATUS
NPMiniConnect(
	__in PFLT_PORT ClientPort,
	__in PVOID ServerPortCookie,
	__in_bcount(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID* ConnectionCookie
)
{
	DbgPrint("[mini-filter] NPMiniConnect");
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	ASSERT(gClientPort == NULL);
	gClientPort = ClientPort;
	return STATUS_SUCCESS;
}

//與user application Disconect
VOID
NPMiniDisconnect(
	__in_opt PVOID ConnectionCookie
)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	DbgPrint("[mini-filter] NPMiniDisconnect");

	//  Close our handle
	FltCloseClientPort(gFilterHandle, &gClientPort);
}

NTSTATUS
NPMiniMessage(
	__in PVOID ConnectionCookie,
	__in_bcount_opt(InputBufferSize) PVOID InputBuffer,
	__in ULONG InputBufferSize,
	__out_bcount_part_opt(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	__in ULONG OutputBufferSize,
	__out PULONG ReturnOutputBufferLength
)
{

	NPMINI_COMMAND command;
	NTSTATUS status;

	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);

	DbgPrint("[mini-filter] NPMiniMessage");

	//                      **** PLEASE READ ****    
	//  The INPUT and OUTPUT buffers are raw user mode addresses.  The filter
	//  manager has already done a ProbedForRead (on InputBuffer) and
	//  ProbedForWrite (on OutputBuffer) which guarentees they are valid
	//  addresses based on the access (user mode vs. kernel mode).  The
	//  minifilter does not need to do their own probe.
	//  The filter manager is NOT doing any alignment checking on the pointers.
	//  The minifilter must do this themselves if they care (see below).
	//  The minifilter MUST continue to use a try/except around any access to
	//  these buffers.    

	if ((InputBuffer != NULL) &&
		(InputBufferSize >= (FIELD_OFFSET(COMMAND_MESSAGE, Mode) +
			sizeof(NPMINI_COMMAND)))) {

		try {
			//  Probe and capture input message: the message is raw user mode
			//  buffer, so need to protect with exception handler
			command = ((PCOMMAND_MESSAGE)InputBuffer)->Mode;

		} except(EXCEPTION_EXECUTE_HANDLER) {
			return GetExceptionCode();
		}

		switch (command) {
			//開放規則
		case ENUM_ADD_RULE:
		{
			//KdBreakPoint();
			DbgPrint("[mini-filter] ENUM_ADD_RULE");
			BOOLEAN bSucc = AddMiniFilterRuleInfo(InputBuffer, InputBufferSize);
			if (bSucc == FALSE)
			{
				status = STATUS_UNSUCCESSFUL;
			}
			else {
				status = STATUS_SUCCESS;
			}
			break;
		}
		//阻擋規則
		case ENUM_REMOVE_RULE:
		{
			DbgPrint("[mini-filter] ENUM_REMOVE_RULE");
			BOOLEAN bSucc = UninitMinFilterRuleInfo();
			if (bSucc == FALSE)
			{
				status = STATUS_UNSUCCESSFUL;
			}
			else {
				status = STATUS_SUCCESS;
			}
			break;
		}

		default:
			DbgPrint("[mini-filter] default");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
	}
	else {

		status = STATUS_INVALID_PARAMETER;
	}

	return status;
}

LIST_ENTRY	g_MiniFilterRuleList = { 0 };

KSPIN_LOCK  g_MiniFilterRuleLock = 0;

BOOLEAN InitMinFilterRuleInfo()
{
	InitializeListHead(&g_MiniFilterRuleList);
	KeInitializeSpinLock(&g_MiniFilterRuleLock);
	return TRUE;
}

BOOLEAN UninitMinFilterRuleInfo()
{
	do
	{
		KIRQL	OldIRQL = 0;
		PLIST_ENTRY pInfo = NULL;
		PMiniFilterFileInfoLIST pRule = NULL;
		if (g_MiniFilterRuleList.Blink == NULL ||
			g_MiniFilterRuleList.Flink == NULL)
		{
			break;
		}
		KeAcquireSpinLock(&g_MiniFilterRuleLock, &OldIRQL);
		while (!IsListEmpty(&g_MiniFilterRuleList))
		{
			pInfo = RemoveHeadList(&g_MiniFilterRuleList);
			if (pInfo == NULL)
			{
				break;
			}
			pRule = CONTAINING_RECORD(pInfo, MiniFilterFileInfoLIST, m_linkPointer);
			ExFreePoolWithTag(pRule, L"UninitMinFilterRuleInfo");
			pRule = NULL;
			pInfo = NULL;
		}
		KeReleaseSpinLock(&g_MiniFilterRuleLock, OldIRQL);
	} while (FALSE);
	return TRUE;
}

BOOLEAN AddMiniFilterRuleInfo(PVOID pBuf, ULONG uLen)
{
	BOOLEAN bSucc = FALSE;
	PMiniFilterFileInfo	pRuleInfo = NULL;
	do
	{
		PMiniFilterFileInfoLIST pRuleNode = NULL;
		KIRQL	OldIRQL = 0;
		pRuleInfo = (PMiniFilterFileInfo)pBuf;
		if (pRuleInfo == NULL)
		{
			break;
		}
		if (uLen < sizeof(MiniFilterFileInfo))
		{
			break;
		}
		pRuleNode = (PMiniFilterFileInfoLIST)ExAllocatePoolWithTag(NonPagedPool, sizeof(MiniFilterFileInfoLIST), "AddMiniFilterRuleInfo");
		if (pRuleNode == NULL)
		{
			break;
		}
		memset(pRuleNode, 0, sizeof(MiniFilterFileInfoLIST));
		if (pRuleInfo->FileName) {
			pRuleNode->m_MiniFilterFileInfo.FileName = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, strlen(pRuleInfo->FileName) + 1, 'MySt');
			if (pRuleNode->m_MiniFilterFileInfo.FileName) InitializeAnsiString(pRuleNode->m_MiniFilterFileInfo.FileName, pRuleInfo->FileName);
		}

		KeAcquireSpinLock(&g_MiniFilterRuleLock, &OldIRQL);
		InsertHeadList(&g_MiniFilterRuleList, &pRuleNode->m_linkPointer);
		KeReleaseSpinLock(&g_MiniFilterRuleLock, OldIRQL);
		bSucc = TRUE;
		break;
	} while (FALSE);
	return bSucc;
}

BOOLEAN IsHitMinifilterRuleFileName(PCHAR FilePath) {
	BOOLEAN bIsHit = FALSE;
	do
	{

		KIRQL	OldIRQL = 0;
		PLIST_ENTRY	pEntry = NULL;
		if (g_MiniFilterRuleList.Blink == NULL ||
			g_MiniFilterRuleList.Flink == NULL)
		{
			DbgPrint("q");
			break;
		}

		KeAcquireSpinLock(&g_MiniFilterRuleLock, &OldIRQL);
		pEntry = g_MiniFilterRuleList.Flink;
		while (pEntry != &g_MiniFilterRuleList)
		{
			PMiniFilterFileInfoLIST pInfo = CONTAINING_RECORD(pEntry, MiniFilterFileInfoLIST, m_linkPointer);

			if (FilePath != NULL) {
				if (pInfo->m_MiniFilterFileInfo.FileName != NULL) {
					if (strstr(FilePath, pInfo->m_MiniFilterFileInfo.FileName) != NULL) {
						bIsHit = TRUE;
						break;
					}
				}
			}

			pEntry = pEntry->Flink;
		}
		KeReleaseSpinLock(&g_MiniFilterRuleLock, OldIRQL);
	} while (FALSE);
	return bIsHit;
}

FLT_PREOP_CALLBACK_STATUS FltDirCtrlPreOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	//if (!IsDriverEnabled())
	//	return FLT_PREOP_SUCCESS_NO_CALLBACK;

	LogInfo("%wZ", &Data->Iopb->TargetFileObject->FileName);

	if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)
	{
	case FileIdFullDirectoryInformation:
	case FileIdBothDirectoryInformation:
	case FileBothDirectoryInformation:
	case FileDirectoryInformation:
	case FileFullDirectoryInformation:
	case FileNamesInformation:
		break;
	default:
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS FltDirCtrlPostOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	PFLT_PARAMETERS params = &Data->Iopb->Parameters;
	PFLT_FILE_NAME_INFORMATION fltName;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	//if (!IsDriverEnabled())
		//return FLT_POSTOP_FINISHED_PROCESSING;

	if (!NT_SUCCESS(Data->IoStatus.Status))
		return FLT_POSTOP_FINISHED_PROCESSING;

	LogInfo("%wZ", &Data->Iopb->TargetFileObject->FileName);

	//if (IsProcessExcluded(PsGetCurrentProcessId()))
	//{
	//	LogTrace("Operation is skipped for excluded process");
	//	return FLT_POSTOP_FINISHED_PROCESSING;
	//}

	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &fltName);
	if (!NT_SUCCESS(status))
	{
		LogWarning("FltGetFileNameInformation() failed with code:%08x", status);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	__try
	{
		status = STATUS_SUCCESS;

		// 这六个是结构体不一样, 其他都一样, 目的是把每个不同的 information 都删掉
		// 这个好像是暂时的, 就算删掉, 再一次获取的时候还是会恢复正常? 这说明还有更底层的地方, 或许删掉那个就无法恢复了.
		switch (params->DirectoryControl.QueryDirectory.FileInformationClass)
		{
		case FileFullDirectoryInformation:
			status = CleanFileFullDirectoryInformation((PFILE_FULL_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer, fltName);
			break;
		case FileBothDirectoryInformation:
			status = CleanFileBothDirectoryInformation((PFILE_BOTH_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer, fltName);
			break;
		case FileDirectoryInformation:
			status = CleanFileDirectoryInformation((PFILE_DIRECTORY_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer, fltName);
			break;
		case FileIdFullDirectoryInformation:
			status = CleanFileIdFullDirectoryInformation((PFILE_ID_FULL_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer, fltName);
			break;
		case FileIdBothDirectoryInformation:
			status = CleanFileIdBothDirectoryInformation((PFILE_ID_BOTH_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer, fltName);
			break;
		case FileNamesInformation:
			status = CleanFileNamesInformation((PFILE_NAMES_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer, fltName);
			break;
		}

		Data->IoStatus.Status = status;
	}
	__finally
	{
		FltReleaseFileNameInformation(fltName);
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}

// 总体原理就是 A -> B -> C ---> A -> C 或者 A B C D -> B C D
NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName)
{
	PFILE_FULL_DIR_INFORMATION nextInfo, prevInfo = NULL;
	UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN matched, search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		// 从 info 获取 name
		fileName.Buffer = info->FileName;
		fileName.Length = (USHORT)info->FileNameLength;
		fileName.MaximumLength = (USHORT)info->FileNameLength;

		if (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			matched = CheckExcludeListDirFile(g_excludeDirectoryContext, &fltName->Name, &fileName);
		else
			matched = CheckExcludeListDirFile(g_excludeFileContext, &fltName->Name, &fileName);

		if (matched)
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					prevInfo->NextEntryOffset += info->NextEntryOffset;
					offset = info->NextEntryOffset;
				}
				else
				{
					prevInfo->NextEntryOffset = 0;
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_FULL_DIR_INFORMATION), 0);
			}
			// 如果第一个就是匹配的
			else
			{
				// 如果还有别的文件
				if (info->NextEntryOffset != 0)
				{
					nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					// 直到 nextInfo 是最后一个结点
					while (nextInfo->NextEntryOffset != 0)
					{
						moveLength += nextInfo->NextEntryOffset;
						nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}
					// 移动块大小 = 从第二个结点开始到最后一个结点距离 + 最后结点的长度
					moveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					// 下一个文件信息放到 -> 需要隐藏的文件信息上面 -> A B C D E -> B C D E
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);//continue
				}
				// 没有的话直接 return 没有啥东西
				else
				{
					status = STATUS_NO_MORE_ENTRIES;
					retn = TRUE;
				}
			}
			LogTrace("Removed from query: %wZ\\%wZ", &fltName->Name, &fileName);

			if (retn)
				return status;

			info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		// 如果不匹配就下一个
		offset = info->NextEntryOffset;
		prevInfo = info;
		info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		// 没有下一个就停
		if (offset == 0)
			search = FALSE;
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileBothDirectoryInformation(PFILE_BOTH_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName)
{
	PFILE_BOTH_DIR_INFORMATION nextInfo, prevInfo = NULL;
	UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN matched, search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		fileName.Buffer = info->FileName;
		fileName.Length = (USHORT)info->FileNameLength;
		fileName.MaximumLength = (USHORT)info->FileNameLength;

		if (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			matched = CheckExcludeListDirFile(g_excludeDirectoryContext, &fltName->Name, &fileName);
		else
			matched = CheckExcludeListDirFile(g_excludeFileContext, &fltName->Name, &fileName);

		if (matched)
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					prevInfo->NextEntryOffset += info->NextEntryOffset;
					offset = info->NextEntryOffset;
				}
				else
				{
					prevInfo->NextEntryOffset = 0;
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_BOTH_DIR_INFORMATION), 0);
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					nextInfo = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0)
					{
						moveLength += nextInfo->NextEntryOffset;
						nextInfo = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);//continue
				}
				else
				{
					status = STATUS_NO_MORE_ENTRIES;
					retn = TRUE;
				}
			}

			LogTrace("Removed from query: %wZ\\%wZ", &fltName->Name, &fileName);

			if (retn)
				return status;

			info = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset;
		prevInfo = info;
		info = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE;
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileDirectoryInformation(PFILE_DIRECTORY_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName)
{
	PFILE_DIRECTORY_INFORMATION nextInfo, prevInfo = NULL;
	UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN matched, search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		fileName.Buffer = info->FileName;
		fileName.Length = (USHORT)info->FileNameLength;
		fileName.MaximumLength = (USHORT)info->FileNameLength;

		if (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			matched = CheckExcludeListDirFile(g_excludeDirectoryContext, &fltName->Name, &fileName);
		else
			matched = CheckExcludeListDirFile(g_excludeFileContext, &fltName->Name, &fileName);

		if (matched)
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					prevInfo->NextEntryOffset += info->NextEntryOffset;
					offset = info->NextEntryOffset;
				}
				else
				{
					prevInfo->NextEntryOffset = 0;
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_DIRECTORY_INFORMATION), 0);
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					nextInfo = (PFILE_DIRECTORY_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0)
					{
						moveLength += nextInfo->NextEntryOffset;
						nextInfo = (PFILE_DIRECTORY_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_DIRECTORY_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);//continue
				}
				else
				{
					status = STATUS_NO_MORE_ENTRIES;
					retn = TRUE;
				}
			}

			LogTrace("Removed from query: %wZ\\%wZ", &fltName->Name, &fileName);

			if (retn)
				return status;

			info = (PFILE_DIRECTORY_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset;
		prevInfo = info;
		info = (PFILE_DIRECTORY_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE;
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileIdFullDirectoryInformation(PFILE_ID_FULL_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName)
{
	PFILE_ID_FULL_DIR_INFORMATION nextInfo, prevInfo = NULL;
	UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN matched, search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		fileName.Buffer = info->FileName;
		fileName.Length = (USHORT)info->FileNameLength;
		fileName.MaximumLength = (USHORT)info->FileNameLength;

		if (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			matched = CheckExcludeListDirFile(g_excludeDirectoryContext, &fltName->Name, &fileName);
		else
			matched = CheckExcludeListDirFile(g_excludeFileContext, &fltName->Name, &fileName);

		if (matched)
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					prevInfo->NextEntryOffset += info->NextEntryOffset;
					offset = info->NextEntryOffset;
				}
				else
				{
					prevInfo->NextEntryOffset = 0;
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_ID_FULL_DIR_INFORMATION), 0);
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					nextInfo = (PFILE_ID_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0)
					{
						moveLength += nextInfo->NextEntryOffset;
						nextInfo = (PFILE_ID_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_ID_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);//continue
				}
				else
				{
					status = STATUS_NO_MORE_ENTRIES;
					retn = TRUE;
				}
			}

			LogTrace("Removed from query: %wZ\\%wZ", &fltName->Name, &fileName);

			if (retn)
				return status;

			info = (PFILE_ID_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset;
		prevInfo = info;
		info = (PFILE_ID_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE;
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileIdBothDirectoryInformation(PFILE_ID_BOTH_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName)
{
	PFILE_ID_BOTH_DIR_INFORMATION nextInfo, prevInfo = NULL;
	UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN matched, search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		fileName.Buffer = info->FileName;
		fileName.Length = (USHORT)info->FileNameLength;
		fileName.MaximumLength = (USHORT)info->FileNameLength;

		if (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			matched = CheckExcludeListDirFile(g_excludeDirectoryContext, &fltName->Name, &fileName);
		else
			matched = CheckExcludeListDirFile(g_excludeFileContext, &fltName->Name, &fileName);

		if (matched)
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					prevInfo->NextEntryOffset += info->NextEntryOffset;
					offset = info->NextEntryOffset;
				}
				else
				{
					prevInfo->NextEntryOffset = 0;
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_ID_BOTH_DIR_INFORMATION), 0);
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					nextInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0)
					{
						moveLength += nextInfo->NextEntryOffset;
						nextInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_ID_BOTH_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);//continue
				}
				else
				{
					status = STATUS_NO_MORE_ENTRIES;
					retn = TRUE;
				}
			}

			LogTrace("Removed from query: %wZ\\%wZ", &fltName->Name, &fileName);

			if (retn)
				return status;

			info = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset;
		prevInfo = info;
		info = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE;
	} while (search);

	return status;
}

NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName)
{
	PFILE_NAMES_INFORMATION nextInfo, prevInfo = NULL;
	UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		fileName.Buffer = info->FileName;
		fileName.Length = (USHORT)info->FileNameLength;
		fileName.MaximumLength = (USHORT)info->FileNameLength;

		//TODO: check, can there be directories?
		if (CheckExcludeListDirFile(g_excludeFileContext, &fltName->Name, &fileName))
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					prevInfo->NextEntryOffset += info->NextEntryOffset;
					offset = info->NextEntryOffset;
				}
				else
				{
					prevInfo->NextEntryOffset = 0;
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_NAMES_INFORMATION), 0);
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					nextInfo = (PFILE_NAMES_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0)
					{
						moveLength += nextInfo->NextEntryOffset;
						nextInfo = (PFILE_NAMES_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);//continue
				}
				else
				{
					status = STATUS_NO_MORE_ENTRIES;
					retn = TRUE;
				}
			}

			LogTrace("Removed from query: %wZ\\%wZ", &fltName->Name, &fileName);

			if (retn)
				return status;

			info = (PFILE_NAMES_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset;
		prevInfo = info;
		info = (PFILE_NAMES_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE;
	} while (search);

	return STATUS_SUCCESS;
}





NTSTATUS InitializeFSMiniFilter(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	UNICODE_STRING str;
	UINT32 i;
	ExcludeEntryId id;

	// Initialize and fill exclude file\dir lists 

	status = InitializeExcludeListContext(&g_excludeFileContext, ExcludeFile);
	if (!NT_SUCCESS(status))
	{
		LogError("Exclude file list initialization failed with code:%08x", status);
		return status;
	}

	status = InitializeExcludeListContext(&g_excludeDirectoryContext, ExcludeDirectory);
	if (!NT_SUCCESS(status))
	{
		LogError("Exclude file list initialization failed with code:%08x", status);
		DestroyExcludeListContext(g_excludeFileContext);
		return status;
	}

	LogTrace("Initialization is completed");
	return status;
}

NTSTATUS DestroyFSMiniFilter()
{
	DestroyExcludeListContext(g_excludeFileContext);
	DestroyExcludeListContext(g_excludeDirectoryContext);

	LogTrace("Deitialization is completed");
	return STATUS_SUCCESS;
}

NTSTATUS AddHiddenFile(PUNICODE_STRING FilePath, PULONGLONG ObjId)
{
	const USHORT maxBufSize = FilePath->Length + NORMALIZE_INCREAMENT;
	UNICODE_STRING normalized;
	NTSTATUS status;

	normalized.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, maxBufSize, FSFILTER_ALLOC_TAG);
	normalized.Length = 0;
	normalized.MaximumLength = maxBufSize;

	if (!normalized.Buffer)
	{
		LogWarning("Error, can't allocate buffer");
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	status = NormalizeDevicePath(FilePath, &normalized);
	if (!NT_SUCCESS(status))
	{
		LogWarning("Path normalization failed with code:%08x, path:%wZ", status, FilePath);
		ExFreePoolWithTag(normalized.Buffer, FSFILTER_ALLOC_TAG);
		return status;
	}

	status = AddExcludeListFile(g_excludeFileContext, &normalized, ObjId, 0);
	if (NT_SUCCESS(status))
		LogTrace("Added hidden file:%wZ", &normalized);
	else
		LogTrace("Adding hidden file failed with code:%08x, path:%wZ", status, &normalized);

	ExFreePoolWithTag(normalized.Buffer, FSFILTER_ALLOC_TAG);

	return status;
}

NTSTATUS RemoveHiddenFile(ULONGLONG ObjId)
{
	NTSTATUS status = RemoveExcludeListEntry(g_excludeFileContext, ObjId);
	if (NT_SUCCESS(status))
		LogTrace("Hidden file is removed, id:%lld", ObjId);
	else
		LogTrace("Can't remove hidden file, code:%08x, id:%lld", status, ObjId);

	return status;
}

NTSTATUS RemoveAllHiddenFiles()
{
	NTSTATUS status = RemoveAllExcludeListEntries(g_excludeFileContext);
	if (NT_SUCCESS(status))
		LogTrace("All hidden files are removed");
	else
		LogTrace("Can't remove all hidden files, code:%08x", status);

	return status;
}

NTSTATUS AddHiddenDir(PUNICODE_STRING DirPath, PULONGLONG ObjId)
{
	const USHORT maxBufSize = DirPath->Length + NORMALIZE_INCREAMENT;
	UNICODE_STRING normalized;
	NTSTATUS status;

	normalized.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, maxBufSize, FSFILTER_ALLOC_TAG);
	normalized.Length = 0;
	normalized.MaximumLength = maxBufSize;

	if (!normalized.Buffer)
	{
		LogWarning("Error, can't allocate buffer");
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	status = NormalizeDevicePath(DirPath, &normalized);
	if (!NT_SUCCESS(status))
	{
		LogWarning("Path normalization failed with code:%08x, path:%wZ\n", status, DirPath);
		ExFreePoolWithTag(normalized.Buffer, FSFILTER_ALLOC_TAG);
		return status;
	}

	status = AddExcludeListDirectory(g_excludeDirectoryContext, &normalized, ObjId, 0);
	if (NT_SUCCESS(status))
		LogTrace("Added hidden dir:%wZ", &normalized);
	else
		LogTrace("Adding hidden dir failed with code:%08x, path:%wZ", status, &normalized);

	ExFreePoolWithTag(normalized.Buffer, FSFILTER_ALLOC_TAG);

	return status;
}

NTSTATUS RemoveHiddenDir(ULONGLONG ObjId)
{
	NTSTATUS status = RemoveExcludeListEntry(g_excludeDirectoryContext, ObjId);
	if (NT_SUCCESS(status))
		LogTrace("Hidden dir is removed, id:%lld", ObjId);
	else
		LogTrace("Can't remove hidden dir, code:%08x, id:%lld", status, ObjId);

	return status;
}

NTSTATUS RemoveAllHiddenDirs()
{
	NTSTATUS status = RemoveAllExcludeListEntries(g_excludeDirectoryContext);
	if (NT_SUCCESS(status))
		LogTrace("All hidden dirs are removed");
	else
		LogTrace("Can't remove all hidden dirs, code:%08x", status);

	return status;
}
