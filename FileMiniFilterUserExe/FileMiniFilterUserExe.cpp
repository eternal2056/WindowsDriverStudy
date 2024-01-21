#include "FileMiniFilterUserExe.h"
#include <iostream>
using namespace std;

int(__stdcall* pNPSendMessage)(PVOID pInBuffer);
int(__stdcall* pInitialCommunicationPort)(VOID);

CNPApp::CNPApp()
{
	m_hModule = NULL;
	LoadNPminifilterDll();
}

CNPApp::~CNPApp()
{
	if (m_hModule) {
		FreeLibrary(m_hModule);
	}
}
bool CNPApp::LoadNPminifilterDll(void)
{
	m_hModule = LoadLibrary("FileMiniFilterUserDll.dll");
	printf("USER: m_hModule: %p\n", m_hModule);
	printf("USER: LoadNPminifilterDll\n");
	if (m_hModule != NULL) {
		printf("USER: LoadNPminifilterDll, FileMiniFilterUserDll: %p\n", GetModuleHandle("FileMiniFilterUserDll.dll"));
		printf("USER: LoadNPminifilterDll, NPSendMessage: %p\n", GetProcAddress(::GetModuleHandle("FileMiniFilterUserDll.dll"), "NPSendMessage"));
		printf("USER: LoadNPminifilterDll, InitialCommunicationPort: %p\n", GetProcAddress(::GetModuleHandle("FileMiniFilterUserDll.dll"), "InitialCommunicationPort"));
		printf("USER: LoadNPminifilterDll, LoadLibraryA: %p\n", GetProcAddress(::GetModuleHandle("kernel32.dll"), "LoadLibraryA"));
		pNPSendMessage = (int(__stdcall*)(PVOID)) GetProcAddress(GetModuleHandle("FileMiniFilterUserDll.dll"), "NPSendMessage");
		printf("USER: LoadNPminifilterDll, pNPSendMessage: %p\n", pNPSendMessage);
		if (!pNPSendMessage) {
			return false;
		}
		return true;
	}
	return false;
}

void CNPApp::NPMessage(COMMAND_MESSAGE data)
{
	if (m_hModule == NULL) {
		if (LoadNPminifilterDll() == false) {
			return;
		}
	}
	printf("USER: NPMessage\n");
	pNPSendMessage((PVOID)(&data));
}

void main(void)
{
	CNPApp ControlObj;
	char input;
	while (true) {
		cout << "Enter 'a' for PASS MODE, 'b' for BLOCKMODE or 'q' to EXIT" << endl;
		cin >> input;
		if (input == 'a' || input == 'A') {
			COMMAND_MESSAGE data;
			data.Command = ENUM_PASS;
			ControlObj.NPMessage(data);
			printf("==>NOTEPAD.EXE PASS MODE\n");
		}
		else if (input == 'b' || input == 'B') {
			COMMAND_MESSAGE data;
			data.Command = ENUM_BLOCK;
			ControlObj.NPMessage(data);
			printf("==>NOTEPAD.EXE BLOCK MODE\n");
		}
		else if (input == 'q' || input == 'Q') {
			printf("EXIT\n");
			break;
		}
		else {
			printf("Wrong Parameter!!!\n");
		}
	};

	system("pause");
}