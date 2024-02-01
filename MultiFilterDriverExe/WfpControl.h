#pragma once
#include <Windows.h>
#pragma pack(push)
#pragma pack(1)
typedef struct _tagWfp_NetInfo
{
	USHORT      m_uSrcPort;	//源端口
	USHORT      m_uRemotePort;	//目标端口
	ULONG       m_ulSrcIPAddr; //源地址
	ULONG       m_ulRemoteIPAddr; //目标地址
	ULONG       m_ulNetWorkType; //协议
	USHORT		m_uDirection;//数据包的方向，0表示发送，1表示接收

} ST_WFP_NETINFO, * PST_WFP_NETINFO;
#pragma pack(pop)

#define IOCTL_WFP_SAMPLE_ADD_RULE CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_WFP_SAMPLE_REMOVE_RULE CTL_CODE(FILE_DEVICE_UNKNOWN,0x802,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)

void WfpMain(int argc, CHAR* argv[]);