#include "FileMiniFilter.h"
#include <iostream>
using namespace std;
std::vector<std::string> forbidExeListTemp;
int(__stdcall* pNPSendMessage)(PVOID pInBuffer);
int(__stdcall* pInitialCommunicationPort)(VOID);

HANDLE g_hPort;

CNPApp::CNPApp()
{
	m_hModule = NULL;
	//LoadNPminifilterDll();
}

CNPApp::~CNPApp()
{
	if (m_hModule) {
		//FreeLibrary(m_hModule);
	}
}
bool CNPApp::LoadNPminifilterDll(void)
{
	//m_hModule = LoadLibrary("FileMiniFilterUserDll.dll");
	//printf("USER: m_hModule: %p\n", m_hModule);
	//printf("USER: LoadNPminifilterDll\n");
	//if (m_hModule != NULL) {
	//	printf("USER: LoadNPminifilterDll, FileMiniFilterUserDll: %p\n", GetModuleHandle("FileMiniFilterUserDll.dll"));
	//	printf("USER: LoadNPminifilterDll, NPSendMessage: %p\n", GetProcAddress(::GetModuleHandle("FileMiniFilterUserDll.dll"), "NPSendMessage"));
	//	printf("USER: LoadNPminifilterDll, InitialCommunicationPort: %p\n", GetProcAddress(::GetModuleHandle("FileMiniFilterUserDll.dll"), "InitialCommunicationPort"));
	//	printf("USER: LoadNPminifilterDll, LoadLibraryA: %p\n", GetProcAddress(::GetModuleHandle("kernel32.dll"), "LoadLibraryA"));
	//	pNPSendMessage = (int(__stdcall*)(PVOID)) GetProcAddress(GetModuleHandle("FileMiniFilterUserDll.dll"), "NPSendMessage");
	//	printf("USER: LoadNPminifilterDll, pNPSendMessage: %p\n", pNPSendMessage);
	//	if (!pNPSendMessage) {
	//		return false;
	//	}
	//	return true;
	//}
	return false;
}

void CNPApp::NPMessage(COMMAND_MESSAGE* data)
{
	//if (m_hModule == NULL) {
	//	if (LoadNPminifilterDll() == false) {
	//		return;
	//	}
	//}
	//printf("USER: NPMessage\n");
	NPSendMessage((PVOID)(data));
}

void FileMiniFilterMain(int argc, CHAR* argv[])
{
	InitialCommunicationPort();
	CNPApp ControlObj;

	std::string param2 = argv[2];
	if (param2 == "AddExe") {
		COMMAND_MESSAGE* data = new COMMAND_MESSAGE;
		*data = { 0 };
		std::string param3 = argv[3];
		data->FileName = new char[param3.length() + 1]; // +1 用于 null 终止符
		strcpy_s(data->FileName, param3.length() + 1, param3.c_str());
		ControlObj.NPMessage(data);
	}
	if (param2 == "AddExeList") {

		for (auto& forbidExe : forbidExeListTemp) {
			COMMAND_MESSAGE* data = new COMMAND_MESSAGE;
			*data = { 0 };
			data->FileName = new char[forbidExe.length() + 1]; // +1 用于 null 终止符
			strcpy_s(data->FileName, forbidExe.length() + 1, forbidExe.c_str());
			ControlObj.NPMessage(data);
		}
	}

	if (param2 == "RemoveRule") {
		COMMAND_MESSAGE* data = new COMMAND_MESSAGE;
		*data = { 0 };
		data->Mode = ENUM_REMOVE_RULE;
		ControlObj.NPMessage(data);
	}
}

int InitialCommunicationPort(void)
{
	printf("DLL: InitialCommunicationPort\n");
	DWORD hResult = FilterConnectCommunicationPort(
		NPMINI_PORT_NAME,
		0,
		NULL,
		0,
		NULL,
		&g_hPort);

	if (hResult != S_OK) {
		return hResult;
	}
	return 0;
}

int NPSendMessage(PVOID InputBuffer)
{
	printf("DLL: NPSendMessage\n");
	DWORD bytesReturned = 0;
	DWORD hResult = 0;
	PCOMMAND_MESSAGE commandMessage = (PCOMMAND_MESSAGE)InputBuffer;
	hResult = FilterSendMessage(
		g_hPort,
		commandMessage,
		sizeof(COMMAND_MESSAGE),
		NULL,
		NULL,
		&bytesReturned);

	if (hResult != S_OK) {
		return hResult;
	}
	return 0;
}