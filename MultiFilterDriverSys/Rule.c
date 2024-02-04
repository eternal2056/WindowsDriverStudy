#include "fltkernel.h"
#include "Rule.h"


LIST_ENTRY	g_WfpRuleList = { 0 };

KSPIN_LOCK  g_RuleLock = 0;


BOOLEAN InitRuleInfo()
{
	InitializeListHead(&g_WfpRuleList);
	KeInitializeSpinLock(&g_RuleLock);
	return TRUE;
}

BOOLEAN UninitRuleInfo()
{
	do
	{
		KIRQL	OldIRQL = 0;
		PLIST_ENTRY pInfo = NULL;
		PST_WFP_NETINFOLIST pRule = NULL;
		if (g_WfpRuleList.Blink == NULL ||
			g_WfpRuleList.Flink == NULL)
		{
			break;
		}
		KeAcquireSpinLock(&g_RuleLock, &OldIRQL);
		while (!IsListEmpty(&g_WfpRuleList))
		{
			pInfo = RemoveHeadList(&g_WfpRuleList);
			if (pInfo == NULL)
			{
				break;
			}
			pRule = CONTAINING_RECORD(pInfo, ST_WFP_NETINFOLIST, m_linkPointer);
			ExFreePoolWithTag(pRule, WFP_TAG);
			pRule = NULL;
			pInfo = NULL;
		}
		KeReleaseSpinLock(&g_RuleLock, OldIRQL);
	} while (FALSE);
	return TRUE;
}

void InitializeAnsiString(CHAR* targetString, const CHAR* sourceString) {
	// 使用字符串处理函数 strncpy 将 sourceString 复制到 targetString
	strncpy(targetString, sourceString, sizeof(sourceString) + 1);

	// 确保字符串以 null 结尾
	targetString[sizeof(sourceString)] = '\0';
}

BOOLEAN AddNetRuleInfo(PVOID pBuf, ULONG uLen)
{
	BOOLEAN bSucc = FALSE;
	PST_WFP_NETINFO	pRuleInfo = NULL;
	do
	{
		PST_WFP_NETINFOLIST pRuleNode = NULL;
		KIRQL	OldIRQL = 0;
		pRuleInfo = (PST_WFP_NETINFO)pBuf;
		if (pRuleInfo == NULL)
		{
			break;
		}
		if (uLen < sizeof(ST_WFP_NETINFO))
		{
			break;
		}
		pRuleNode = (PST_WFP_NETINFOLIST)ExAllocatePoolWithTag(NonPagedPool, sizeof(ST_WFP_NETINFOLIST), WFP_TAG);
		if (pRuleNode == NULL)
		{
			break;
		}
		memset(pRuleNode, 0, sizeof(ST_WFP_NETINFOLIST));
		pRuleNode->m_stWfpNetInfo.m_uSrcPort = pRuleInfo->m_uSrcPort;
		pRuleNode->m_stWfpNetInfo.m_uRemotePort = pRuleInfo->m_uRemotePort;
		pRuleNode->m_stWfpNetInfo.m_ulSrcIPAddr = pRuleInfo->m_ulSrcIPAddr;
		pRuleNode->m_stWfpNetInfo.m_ulRemoteIPAddr = pRuleInfo->m_ulRemoteIPAddr;
		pRuleNode->m_stWfpNetInfo.m_ulNetWorkType = pRuleInfo->m_ulNetWorkType;
		pRuleNode->m_stWfpNetInfo.m_uDirection = pRuleInfo->m_uDirection;

		pRuleNode->m_stWfpNetInfo.m_url = (PCHAR)ExAllocatePoolWithTag(
			NonPagedPool, strlen(pRuleInfo->m_url) + 1, 'MySt');

		if (pRuleNode->m_stWfpNetInfo.m_url) InitializeAnsiString(pRuleNode->m_stWfpNetInfo.m_url, pRuleInfo->m_url);

		KeAcquireSpinLock(&g_RuleLock, &OldIRQL);
		InsertHeadList(&g_WfpRuleList, &pRuleNode->m_linkPointer);
		KeReleaseSpinLock(&g_RuleLock, OldIRQL);
		bSucc = TRUE;
		break;
	} while (FALSE);
	return bSucc;
}

BOOLEAN IsHitRule(ULONG ulRemoteIPAddress)
{
	BOOLEAN bIsHit = FALSE;
	do
	{

		KIRQL	OldIRQL = 0;
		PLIST_ENTRY	pEntry = NULL;
		if (g_WfpRuleList.Blink == NULL ||
			g_WfpRuleList.Flink == NULL)
		{
			DbgPrint("q");
			break;
		}

		KeAcquireSpinLock(&g_RuleLock, &OldIRQL);
		pEntry = g_WfpRuleList.Flink;
		while (pEntry != &g_WfpRuleList)
		{
			PST_WFP_NETINFOLIST pInfo = CONTAINING_RECORD(pEntry, ST_WFP_NETINFOLIST, m_linkPointer);

			if (ulRemoteIPAddress == pInfo->m_stWfpNetInfo.m_ulRemoteIPAddr)
			{

				bIsHit = TRUE;
				break;
			}
			pEntry = pEntry->Flink;
		}
		KeReleaseSpinLock(&g_RuleLock, OldIRQL);
	} while (FALSE);
	return bIsHit;

}
BOOLEAN IsHitRulePort(ULONG wRemotePort)
{
	BOOLEAN bIsHit = FALSE;
	do
	{

		KIRQL	OldIRQL = 0;
		PLIST_ENTRY	pEntry = NULL;
		if (g_WfpRuleList.Blink == NULL ||
			g_WfpRuleList.Flink == NULL)
		{
			DbgPrint("q");
			break;
		}

		KeAcquireSpinLock(&g_RuleLock, &OldIRQL);
		pEntry = g_WfpRuleList.Flink;
		while (pEntry != &g_WfpRuleList)
		{
			PST_WFP_NETINFOLIST pInfo = CONTAINING_RECORD(pEntry, ST_WFP_NETINFOLIST, m_linkPointer);

			if (wRemotePort == pInfo->m_stWfpNetInfo.m_uRemotePort)
			{
				bIsHit = TRUE;
				break;
			}
			pEntry = pEntry->Flink;
		}
		KeReleaseSpinLock(&g_RuleLock, OldIRQL);
	} while (FALSE);
	return bIsHit;

}

BOOLEAN IsHitRuleUrl(PCHAR visibleString) {
	BOOLEAN bIsHit = FALSE;
	do
	{

		KIRQL	OldIRQL = 0;
		PLIST_ENTRY	pEntry = NULL;
		if (g_WfpRuleList.Blink == NULL ||
			g_WfpRuleList.Flink == NULL)
		{
			DbgPrint("q");
			break;
		}

		KeAcquireSpinLock(&g_RuleLock, &OldIRQL);
		pEntry = g_WfpRuleList.Flink;
		while (pEntry != &g_WfpRuleList)
		{
			PST_WFP_NETINFOLIST pInfo = CONTAINING_RECORD(pEntry, ST_WFP_NETINFOLIST, m_linkPointer);


			if (visibleString != NULL) {
				if (pInfo->m_stWfpNetInfo.m_url != NULL) {
					if (strstr(visibleString, pInfo->m_stWfpNetInfo.m_url) != NULL) {
						bIsHit = TRUE;
						break;
					}
				}
			}

			pEntry = pEntry->Flink;
		}
		KeReleaseSpinLock(&g_RuleLock, OldIRQL);
	} while (FALSE);
	return bIsHit;
}