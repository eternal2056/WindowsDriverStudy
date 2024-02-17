#include <windows.h>
#include <iostream>
#include <string>

#define KILLRULE_NTDEVICE_NAME L"\\Device\\KillRuleDrv"
#define KILLRULE_DOSDEVICE_NAME L"\\DosDevices\\KillRuleDrv"
#define KILLRULE_USER_SYMBOLINK L"\\\\.\\KillRuleDrv"

#define MINIFILTER_DEVICE_NAME             L"\\Device\\HiddenGate"
#define MINIFILTER_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\HiddenGate"
#define MINIFILTER_USER_DEVICES_LINK_NAME   L"\\\\.\\HiddenGate"

#define KILLRULE_DRV_TYPE 41828

#define IOCTL_KILLRULE_HEARTBEAT CTL_CODE(KILLRULE_DRV_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_DRIVER CTL_CODE(KILLRULE_DRV_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_NOTIFY CTL_CODE(KILLRULE_DRV_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_MEMORY CTL_CODE(KILLRULE_DRV_TYPE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_HOTKEY CTL_CODE(KILLRULE_DRV_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_STORAGE CTL_CODE(KILLRULE_DRV_TYPE, 0x920, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_OBJECT CTL_CODE(KILLRULE_DRV_TYPE, 0x940, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILLRULE_PROCESS CTL_CODE(KILLRULE_DRV_TYPE, 0x960, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HIDE_WINDOW CTL_CODE(KILLRULE_DRV_TYPE, 0x812, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HID_IOCTL_ADD_HIDDEN_OBJECT              CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 60), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define HID_IOCTL_REMOVE_HIDDEN_OBJECT           CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 61), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define HID_IOCTL_REMOVE_ALL_HIDDEN_OBJECTS      CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 62), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)


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

typedef struct _HidContextInternal {
	HANDLE hdevice;
} HidContextInternal, * PHidContextInternal;

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

typedef struct _MyMessage64
{
	__int64 window_result;			// 执行结果
	__int64 window_handle;			// 窗口句柄
	int window_attributes;				// 窗口属性
}MyMessage64, * PMyMessage64;

void controlHideProcess(HANDLE hDevice, wchar_t* className);