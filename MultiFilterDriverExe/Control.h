﻿#include <windows.h>
#include <iostream>
#include <string>

#define KILLRULE_NTDEVICE_NAME L"\\Device\\KillRuleDrv"
#define KILLRULE_DOSDEVICE_NAME L"\\DosDevices\\KillRuleDrv"
#define KILLRULE_USER_SYMBOLINK L"\\\\.\\KillRuleDrv"

#define KILLRULE_DRV_TYPE 41828

#define IOCTL_KILLRULE_HEARTBEAT CTL_CODE(KILLRULE_DRV_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_DRIVER CTL_CODE(KILLRULE_DRV_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_NOTIFY CTL_CODE(KILLRULE_DRV_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_MEMORY CTL_CODE(KILLRULE_DRV_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_HOTKEY CTL_CODE(KILLRULE_DRV_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_STORAGE CTL_CODE(KILLRULE_DRV_TYPE, 0x920, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_OBJECT CTL_CODE(KILLRULE_DRV_TYPE, 0x940, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_PROCESS CTL_CODE(KILLRULE_DRV_TYPE, 0x960, METHOD_BUFFERED, FILE_ANY_ACCESS)



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

int ControlMain(int argc, CHAR* argv[]);