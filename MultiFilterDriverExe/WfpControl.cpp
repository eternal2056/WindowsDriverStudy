#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "WfpControl.h"
std::vector<std::string> forbidAddressListTemp;

#pragma comment(lib, "Ws2_32.lib")  // 链接 Ws2_32.lib 库

void WfpMain(int argc, CHAR* argv[]) {
	int PortId;
	std::string param2 = argv[2];
	HANDLE hFile = CreateFile((L"\\\\.\\wfp_sample_device"), GENERIC_ALL, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}


	if (param2 == "RemotePort") {
		ST_WFP_NETINFO* Info = new ST_WFP_NETINFO;
		*Info = { 0 };
		std::string param3 = argv[3];
		PortId = std::stoi(param3);
		Info->m_uRemotePort = PortId;
		printf("\n%X\n", PortId);
		DWORD dwNeedSize = 0;
		DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_ADD_RULE, (LPVOID)Info, sizeof(*Info), NULL, 0, &dwNeedSize, NULL);
	}

	if (param2 == "RemoteIp") {
		std::string param3 = argv[3];
		ULONG ulIPAddress = inet_addr(param3.c_str());
		ulIPAddress = ntohl(ulIPAddress);
		ST_WFP_NETINFO* Info = new ST_WFP_NETINFO;
		*Info = { 0 };
		printf("\n%X\n", ulIPAddress);
		Info->m_ulRemoteIPAddr = ulIPAddress;
		DWORD dwNeedSize = 0;
		DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_ADD_RULE, (LPVOID)Info, sizeof(*Info), NULL, 0, &dwNeedSize, NULL);
	}
	if (param2 == "SourceIp") {
		std::string param3 = argv[3];
		ULONG ulIPAddress = inet_addr(param3.c_str());
		ulIPAddress = ntohl(ulIPAddress);
		ST_WFP_NETINFO* Info = new ST_WFP_NETINFO;
		*Info = { 0 };
		Info->m_ulSrcIPAddr = ulIPAddress;
		DWORD dwNeedSize = 0;
		DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_ADD_RULE, (LPVOID)Info, sizeof(*Info), NULL, 0, &dwNeedSize, NULL);
	}

	if (param2 == "RemoveRule") {
		DWORD dwNeedSize = 0;
		DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_REMOVE_RULE, NULL, 0, NULL, 0, &dwNeedSize, NULL);
	}

	if (param2 == "AddressAdd") {
		ST_WFP_NETINFO* Info = new ST_WFP_NETINFO;
		*Info = { 0 };
		std::string param3 = argv[3];
		Info->m_url = new char[param3.length() + 1]; // +1 用于 null 终止符
		// 使用 strcpy 复制字符串的内容
		strcpy_s(Info->m_url, param3.length() + 1, param3.c_str());
		printf("\n%s", Info->m_url);
		DWORD dwNeedSize = 0;
		DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_ADDRESS_ADD, (LPVOID)Info, sizeof(*Info), NULL, 0, &dwNeedSize, NULL);
	}

	if (param2 == "AddressListAdd") {
		for (auto& forbidAddress : forbidAddressListTemp) {
			ST_WFP_NETINFO* Info = new ST_WFP_NETINFO;
			*Info = { 0 };
			Info->m_url = new char[forbidAddress.length() + 1]; // +1 用于 null 终止符
			// 使用 strcpy 复制字符串的内容
			strcpy_s(Info->m_url, forbidAddress.length() + 1, forbidAddress.c_str());
			printf("\n%s", Info->m_url);
			DWORD dwNeedSize = 0;
			DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_ADDRESS_ADD, (LPVOID)Info, sizeof(*Info), NULL, 0, &dwNeedSize, NULL);
		}

	}
	// 读者可以在此基础上增加其他域的过滤，如IP，协议类型、数据包方向等

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
}
