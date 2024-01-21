// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include "windows.h"
#include <stdio.h>
#include <FltUser.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "fltLib.lib")
//#pragma comment(lib, "fltMgr.lib")
//#pragma comment(lib, "ntoskrnl.lib")
#pragma comment(lib, "hal.lib")

extern HANDLE g_hPort;

#define NPMINI_NAME            L"FileMiniFilterDriver"
#define NPMINI_PORT_NAME       L"\\NPMiniPort"

__declspec(dllexport)	int InitialCommunicationPort(void);
__declspec(dllexport)   int NPSendMessage(PVOID InputBuffer);

typedef enum _NPMINI_COMMAND {
	ENUM_PASS = 0,
	ENUM_BLOCK
} NPMINI_COMMAND;

typedef struct _COMMAND_MESSAGE {
	NPMINI_COMMAND 	Command;
} COMMAND_MESSAGE, * PCOMMAND_MESSAGE;
#endif //PCH_H
