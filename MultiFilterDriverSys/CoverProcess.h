#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>
#include "Tools.h"

#define KILLRULE_NTDEVICE_NAME L"\\Device\\KillRuleDrv"
#define KILLRULE_DOSDEVICE_NAME L"\\DosDevices\\KillRuleDrv"
#define KILLRULE_USER_SYMBOLINK L"\\\\.\\KillRuleDrv"

#define KILLRULE_DRV_TYPE 41828
#define KILLRULE_POOLTAG 'OKL'

#define IOCTL_KILLRULE_HEARTBEAT CTL_CODE(KILLRULE_DRV_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_DRIVER CTL_CODE(KILLRULE_DRV_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_NOTIFY CTL_CODE(KILLRULE_DRV_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_MEMORY CTL_CODE(KILLRULE_DRV_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_HOTKEY CTL_CODE(KILLRULE_DRV_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_STORAGE CTL_CODE(KILLRULE_DRV_TYPE, 0x920, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_OBJECT CTL_CODE(KILLRULE_DRV_TYPE, 0x940, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_PROCESS CTL_CODE(KILLRULE_DRV_TYPE, 0x960, METHOD_BUFFERED, FILE_ANY_ACCESS)

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

typedef struct _MY_POINT {
	ULONG Value1;
	ULONG Value2;
} MY_POINT, * PMY_POINT;

typedef struct _MY_DATA {
	ULONG Value1;
	ULONG Value2;
	MY_POINT* MyPoint;
} MY_DATA, * PMY_DATA;

typedef struct _PROCESS_MY {
	ULONG ProcessId;
} PROCESS_MY, * PPROCESS_MY;


// --------------------------------------------------- 以下为失败品 | 谨记 ------------------------------------------ //

typedef struct _NON_PAGED_DEBUG_INFO {
	USHORT      Signature;
	USHORT      Flags;
	ULONG       Size;
	USHORT      Machine;
	USHORT      Characteristics;
	ULONG       TimeDateStamp;
	ULONG       CheckSum;
	ULONG       SizeOfImage;
	ULONGLONG   ImageBase;
} NON_PAGED_DEBUG_INFO, * PNON_PAGED_DEBUG_INFO;

typedef struct _KLDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	PVOID ExceptionTable;
	ULONG ExceptionTableSize;
	// ULONG padding on IA64
	PVOID GpValue;
	PNON_PAGED_DEBUG_INFO NonPagedDebugInfo;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT __Unused5;
	PVOID SectionPointer;
	ULONG CheckSum;
	// ULONG padding on IA64
	PVOID LoadedImports;
	PVOID PatchInformation;
} KLDR_DATA_TABLE_ENTRY, * PKLDR_DATA_TABLE_ENTRY;

typedef VOID(*pMiProcessLoaderEntry)(
	IN PVOID DataTableEntry,
	IN LOGICAL Insert
	);
pMiProcessLoaderEntry MiProcessLoaderEntry;

NTSTATUS CoverProcessDriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
);

VOID CoverProcessDriverUnload(DRIVER_OBJECT* DriverObject);
NTSTATUS
CloseFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
);

NTSTATUS
CreateFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
);
NTSTATUS MainDispatcher(PDEVICE_OBJECT devobj, PIRP irp);