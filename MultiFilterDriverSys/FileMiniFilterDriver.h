#pragma once
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <ntddscsi.h>		
#include "ExcludeList.h"

#define MINIFILTER_DEVICE_NAME             L"\\Device\\HiddenGate"
#define MINIFILTER_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\HiddenGate"

#define HID_IOCTL_ADD_HIDDEN_OBJECT              CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 60), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define HID_IOCTL_REMOVE_HIDDEN_OBJECT           CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 61), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define HID_IOCTL_REMOVE_ALL_HIDDEN_OBJECTS      CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 62), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

enum Hid_ObjectTypes {
	RegKeyObject,
	RegValueObject,
	FsFileObject,
	FsDirObject,
	PsExcludedObject,
	PsProtectedObject,
	PsHiddenObject,
	PsActiveHiddenObject
};

#pragma pack(push, 4)

// Fs/Reg packets

typedef struct _Hid_DriverStatusPacket {
	unsigned short state;
	unsigned short reserved;
} Hid_DriverStatus, * PHid_DriverStatus;

typedef struct _Hid_HideObjectPacket {
	unsigned short objType;
	unsigned short dataSize;
} Hid_HideObjectPacket, * PHid_HideObjectPacket;

typedef struct _Hid_UnhideObjectPacket {
	unsigned short objType;
	unsigned short reserved;
	unsigned long long id;
} Hid_UnhideObjectPacket, * PHid_UnhideObjectPacket;

typedef struct _Hid_UnhideAllObjectsPacket {
	unsigned short objType;
	unsigned short reserved;
} Hid_UnhideAllObjectsPacket, * PHid_UnhideAllObjectsPacket;

// Ps packets

typedef struct _Hid_AddPsObjectPacket {
	unsigned short objType;
	unsigned short dataSize;
	unsigned short inheritType;
	unsigned short applyForProcesses;
} Hid_AddPsObjectPacket, * PHid_AddPsObjectPacket;

typedef struct _Hid_GetPsObjectInfoPacket {
	unsigned short objType;
	unsigned short inheritType;
	unsigned short enable;
	unsigned short reserved;
	unsigned long procId;
} Hid_GetPsObjectInfoPacket, * PHid_GetPsObjectInfoPacket;

typedef Hid_GetPsObjectInfoPacket Hid_SetPsObjectInfoPacket;
typedef Hid_GetPsObjectInfoPacket* PHid_SetPsObjectInfoPacket;

typedef struct _Hid_RemovePsObjectPacket {
	unsigned short objType;
	unsigned short reserved;
	unsigned long long id;
} Hid_RemovePsObjectPacket, * PHid_RemovePsObjectPacket;

typedef struct _Hid_RemoveAllPsObjectsPacket {
	unsigned short objType;
	unsigned short reserved;
} Hid_RemoveAllPsObjectsPacket, * PHid_RemoveAllPsObjectsPacket;

// Result packet

typedef struct _Hid_StatusPacket {
	unsigned int status;
	unsigned int dataSize;
	union {
		unsigned long long id;
		unsigned long state;
	} info;
}  Hid_StatusPacket, * PHid_StatusPacket;

#pragma pack(pop)


#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

#define MINISPY_PORT_NAME								L"\\NPMiniPort"

/**
 * Âà´«¦r¦ê®æ¦¡
 *
 * @param  	UniName			IN UNICODE_STRING
 * @param  	Name				IN OUT char pointor
 * @return  BOOLEAN 		TURE,FALSE
 */
BOOLEAN NPUnicodeStringToChar(PUNICODE_STRING UniName, char Name[]);


/*************************************************************************
	Prototypes
*************************************************************************/

NTSTATUS MiniFilterDriverEntry(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING RegistryPath
);

NTSTATUS
NPInstanceSetup(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_SETUP_FLAGS Flags,
	__in DEVICE_TYPE VolumeDeviceType,
	__in FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
NPInstanceTeardownStart(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
NPInstanceTeardownComplete(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
NPUnload(
	__in FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
NPInstanceQueryTeardown(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
NPPreCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID* CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
NPPostCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
);

NTSTATUS
NPMiniMessage(
	__in PVOID ConnectionCookie,
	__in_bcount_opt(InputBufferSize) PVOID InputBuffer,
	__in ULONG InputBufferSize,
	__out_bcount_part_opt(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	__in ULONG OutputBufferSize,
	__out PULONG ReturnOutputBufferLength
);

NTSTATUS
NPMiniConnect(
	__in PFLT_PORT ClientPort,
	__in PVOID ServerPortCookie,
	__in_bcount(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID* ConnectionCookie
);

VOID
NPMiniDisconnect(
	__in_opt PVOID ConnectionCookie
);
FLT_PREOP_CALLBACK_STATUS FltDirCtrlPreOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_POSTOP_CALLBACK_STATUS FltDirCtrlPostOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags);

//  Assign text sections for each routine.
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MiniFilterDriverEntry)
#pragma alloc_text(PAGE, NPUnload)
#pragma alloc_text(PAGE, NPInstanceQueryTeardown)
#pragma alloc_text(PAGE, NPInstanceSetup)
#pragma alloc_text(PAGE, NPInstanceTeardownStart)
#pragma alloc_text(PAGE, NPInstanceTeardownComplete)
#pragma alloc_text(PAGE, NPPreCreate)
#pragma alloc_text(PAGE, NPMiniConnect)				//for port comunication
#pragma alloc_text(PAGE, NPMiniDisconnect)			//for port comunication
#pragma alloc_text(PAGE, NPMiniMessage)    		//for port comunication		
#endif    

//  operation registration
const static FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE, 0, NPPreCreate, NPPostCreate },
	{ IRP_MJ_DIRECTORY_CONTROL, 0, FltDirCtrlPreOperation, FltDirCtrlPostOperation },
	{ IRP_MJ_OPERATION_END }
};

//  This defines what we want to filter with FltMgr
const static FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	NULL,                               //  Context
	Callbacks,                          //  Operation callbacks

	NPUnload,                           //  MiniFilterUnload

	NPInstanceSetup,                    //  InstanceSetup
	NPInstanceQueryTeardown,            //  InstanceQueryTeardown
	NPInstanceTeardownStart,            //  InstanceTeardownStart
	NPInstanceTeardownComplete,         //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent

};

//  Defines the commands between the utility and the filter
typedef enum _NPMINI_COMMAND {
	ENUM_ADD_RULE = 0,
	ENUM_REMOVE_RULE = 1,
} NPMINI_COMMAND;

typedef struct _COMMAND_MESSAGE {
	char* FileName;
	NPMINI_COMMAND Mode;
} COMMAND_MESSAGE, * PCOMMAND_MESSAGE;


typedef struct _MiniFilterFileInfo
{
	char* FileName;
} MiniFilterFileInfo, * PMiniFilterFileInfo;

typedef struct _MiniFilterFileInfoList
{
	LIST_ENTRY		m_linkPointer;
	MiniFilterFileInfo	m_MiniFilterFileInfo;

}MiniFilterFileInfoLIST, * PMiniFilterFileInfoLIST;

BOOLEAN InitMinFilterRuleInfo();
BOOLEAN UninitMinFilterRuleInfo();
BOOLEAN AddMiniFilterRuleInfo(PVOID pBuf, ULONG uLen);
BOOLEAN IsHitMinifilterRuleFileName(PCHAR FilePath);

NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName);
NTSTATUS CleanFileBothDirectoryInformation(PFILE_BOTH_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName);
NTSTATUS CleanFileDirectoryInformation(PFILE_DIRECTORY_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName);
NTSTATUS CleanFileIdFullDirectoryInformation(PFILE_ID_FULL_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName);
NTSTATUS CleanFileIdBothDirectoryInformation(PFILE_ID_BOTH_DIR_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName);
NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION info, PFLT_FILE_NAME_INFORMATION fltName);

ExcludeContext g_excludeFileContext;
ExcludeContext g_excludeDirectoryContext;

NTSTATUS InitializeFSMiniFilter(PDRIVER_OBJECT DriverObject);
NTSTATUS DestroyFSMiniFilter();

NTSTATUS AddHiddenFile(PUNICODE_STRING FilePath, PULONGLONG ObjId);
NTSTATUS RemoveHiddenFile(ULONGLONG ObjId);
NTSTATUS RemoveAllHiddenFiles();

NTSTATUS AddHiddenDir(PUNICODE_STRING DirPath, PULONGLONG ObjId);
NTSTATUS RemoveHiddenDir(ULONGLONG ObjId);
NTSTATUS RemoveAllHiddenDirs();

NTSTATUS IrpMiniFilterDeviceCreate(PDEVICE_OBJECT  DeviceObject, PIRP  Irp);
NTSTATUS IrpMiniFilterDeviceClose(PDEVICE_OBJECT  DeviceObject, PIRP  Irp);
NTSTATUS IrpMiniFilterDeviceCleanup(PDEVICE_OBJECT  DeviceObject, PIRP  Irp);
NTSTATUS IrpMiniFilterDeviceControlHandler(PDEVICE_OBJECT  DeviceObject, PIRP  Irp);
