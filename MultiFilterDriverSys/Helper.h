﻿#pragma once

#include <Ntddk.h>

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemInterruptInformation = 23,
	SystemExceptionInformation = 33,
	SystemRegistryQuotaInformation = 37,
	SystemLookasideInformation = 45,
	SystemPolicyInformation = 134,
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG           NextEntryOffset;
	ULONG           NumberOfThreads;
	LARGE_INTEGER   Reserved[3];
	LARGE_INTEGER   CreateTime;
	LARGE_INTEGER   UserTime;
	LARGE_INTEGER   KernelTime;
	UNICODE_STRING  ImageName;
	KPRIORITY       BasePriority;
	HANDLE          ProcessId;
	HANDLE          InheritedFromProcessId;
	ULONG           HandleCount;
	UCHAR           Reserved4[4];
	PVOID           Reserved5[11];
	SIZE_T          PeakPagefileUsage;
	SIZE_T          PrivatePageCount;
	LARGE_INTEGER   Reserved6[6];
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY     LoadOrder;
	LIST_ENTRY     MemoryOrder;
	LIST_ENTRY     InitializationOrder;
	PVOID          ModuleBaseAddress;
	PVOID          EntryPoint;
	ULONG          ModuleSize;
	UNICODE_STRING FullModuleName;
	UNICODE_STRING ModuleName;
	ULONG          Flags;
	USHORT         LoadCount;
	USHORT         TlsIndex;
	union {
		LIST_ENTRY Hash;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		} s;
	} u;
	ULONG   TimeStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(
	_In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Inout_   PVOID                    SystemInformation,
	_In_      ULONG                    SystemInformationLength,
	_Out_opt_ PULONG                   ReturnLength
);

NTSYSAPI NTSTATUS NTAPI ZwQueryInformationProcess(
	_In_      HANDLE                    ProcessHandle,
	_In_      PROCESSINFOCLASS          ProcessInformationClass,
	_Out_     PVOID                     ProcessInformation,
	_In_      ULONG                     ProcessInformationLength,
	_Out_opt_ PULONG                    ReturnLength
);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(
	_In_ HANDLE ProcessId,
	_Outptr_ PEPROCESS* Process
);

#define EXHANDLE_TABLE_ENTRY_LOCK_BIT    1

typedef struct _HANDLE_TABLE_ENTRY {
	union {
		VOID* Object;
		ULONG_PTR Value;
	} u1;
	union {
		ULONG GrantedAccess;
		ULONG_PTR NextFreeTableEntry;
	} u2;
} HANDLE_TABLE_ENTRY, * PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE_WIN8 {
	ULONG NextHandleNeedingPool;
	LONG ExtraInfoPages;
	volatile ULONG TableCode;
	struct _EPROCESS* QuotaProcess;
	struct _LIST_ENTRY HandleTableList;
	ULONG UniqueProcessId;
	ULONG Flags;
	EX_PUSH_LOCK HandleContentionEvent;
	EX_PUSH_LOCK HandleTableLock;
	// ... other useless fields
} HANDLE_TABLE_WIN8, * PHANDLE_TABLE_WIN8;

// Windows 7
typedef BOOLEAN(*EX_ENUMERATE_HANDLE_ROUTINE)(
	IN PHANDLE_TABLE_ENTRY HandleTableEntry,
	IN HANDLE Handle,
	IN PVOID EnumParameter
	);

// Windows 8
typedef BOOLEAN(*EX_ENUMERATE_HANDLE_ROUTINE_WIN8)(
	IN PVOID PspCidTable,
	IN PHANDLE_TABLE_ENTRY HandleTableEntry,
	IN HANDLE Handle,
	IN PVOID EnumParameter
	);

NTKERNELAPI
BOOLEAN
ExEnumHandleTable(
	_In_  PVOID HandleTable,
	_In_  EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
	_In_  PVOID EnumParameter,
	_Out_opt_ PHANDLE Handle
);

NTKERNELAPI
VOID
FASTCALL
ExfUnblockPushLock(
	PEX_PUSH_LOCK PushLock,
	PVOID CurrentWaitBlock
);

NTSTATUS QuerySystemInformation(SYSTEM_INFORMATION_CLASS Class, PVOID* InfoBuffer, PSIZE_T InfoSize);
NTSTATUS QueryProcessInformation(PROCESSINFOCLASS Class, HANDLE ProcessId, PVOID* InfoBuffer, PSIZE_T InfoSize);
VOID FreeInformation(PVOID Buffer);

#define NORMALIZE_INCREAMENT (USHORT)0x200

NTSTATUS NormalizeDevicePath(PCUNICODE_STRING Path, PUNICODE_STRING Normalized);

#define _LogMsg(lvl, lvlname, frmt, ...) \
	DbgPrintEx(\
		DPFLTR_IHVDRIVER_ID, \
		lvl, \
		"[" lvlname "] [irql:%Iu,pid:%Iu,tid:%Iu]\thidden!" __FUNCTION__ ": " frmt "\n", \
		KeGetCurrentIrql(), \
		PsGetCurrentProcessId(), \
		PsGetCurrentThreadId(), \
		__VA_ARGS__ \
	)

BOOLEAN IsWin8OrAbove();

#define LogError(frmt,   ...) _LogMsg(DPFLTR_ERROR_LEVEL,   "error",   frmt, __VA_ARGS__)
#define LogWarning(frmt, ...) _LogMsg(DPFLTR_WARNING_LEVEL, "warning", frmt, __VA_ARGS__)
#define LogTrace(frmt,   ...) _LogMsg(DPFLTR_TRACE_LEVEL,   "trace",   frmt, __VA_ARGS__)
#define LogInfo(frmt,    ...) _LogMsg(DPFLTR_INFO_LEVEL,    "info",    frmt, __VA_ARGS__)