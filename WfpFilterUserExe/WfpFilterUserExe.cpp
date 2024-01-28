#include <iostream>
#include "WfpFilterUserExe.h"

void main()
{
	HANDLE hFile = CreateFile(("\\\\.\\wfp_sample_device"), GENERIC_ALL, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}
	ST_WFP_NETINFO Info = { 0 };
	// 读者可以在此基础上增加其他域的过滤，如IP，协议类型、数据包方向等
	Info.m_uRemotePort = 443;
	DWORD dwNeedSize = 0;
	DeviceIoControl(hFile, IOCTL_WFP_SAMPLE_ADD_RULE, (LPVOID)&Info, sizeof(Info), NULL, 0, &dwNeedSize, NULL);
	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
}
