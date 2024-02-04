#pragma once

#include "windows.h"
#include <vector>
#include <string>
#include "windows.h"
#include <stdio.h>
#include <FltUser.h>
#include <iostream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "fltLib.lib")
//#pragma comment(lib, "fltMgr.lib")
//#pragma comment(lib, "ntoskrnl.lib")
#pragma comment(lib, "hal.lib")


#define NPMINI_NAME            L"FileMiniFilterDriver"
#define NPMINI_PORT_NAME       L"\\NPMiniPort"

int InitialCommunicationPort(void);
int NPSendMessage(PVOID InputBuffer);
void FileMiniFilterMain(int argc, CHAR* argv[]);

using namespace std;

typedef enum _NPMINI_COMMAND {
	ENUM_ADD_RULE = 0,
	ENUM_REMOVE_RULE = 1,
} NPMINI_COMMAND;

typedef struct _COMMAND_MESSAGE {
	char* FileName;
	NPMINI_COMMAND Mode;
} COMMAND_MESSAGE, * PCOMMAND_MESSAGE;

class CNPApp
{
public:
	CNPApp();
	virtual ~CNPApp();
	void NPMessage(COMMAND_MESSAGE* data);

private:
	HINSTANCE m_hModule;
	bool LoadNPminifilterDll(void);
};