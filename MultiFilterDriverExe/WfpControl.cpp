#include <iostream>
#include <string>
#include "WfpControl.h"

void WfpMain(int argc, CHAR* argv[]) {
	HANDLE hFile = CreateFile((L"\\\\.\\wfp_sample_device"), GENERIC_ALL, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}
	ST_WFP_NETINFO Info = { 0 };
	int PortId;
	std::string param2 = argv[2];

	if (param2 == "Port") {
		std::string param3 = argv[3];
		PortId = std::stoi(param3);
		Info.m_uRemotePort = PortId;
		DWORD dwNeedSize = 0;
		DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_ADD_RULE, (LPVOID)&Info, sizeof(Info), NULL, 0, &dwNeedSize, NULL);
	}
	if (param2 == "RemoveRule") {
		DWORD dwNeedSize = 0;
		DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_REMOVE_RULE, (LPVOID)&Info, sizeof(Info), NULL, 0, &dwNeedSize, NULL);
	}
	// 读者可以在此基础上增加其他域的过滤，如IP，协议类型、数据包方向等

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
}
