#include "WfpSample.h"
//#include "Fwpmu.h"
#include "Rule.h"
#include "Tools.h"


/*
本例子只是为了介绍WFP的用法，在实际应用中，如果只是根据简单的规则拦截数据包的话，
可以通过添加过滤器的方法实现。
*/
PDEVICE_OBJECT g_pDeviceObj = NULL;

UINT32	g_uFwpsEstablishedCallOutId = 0;
UINT32	g_uFwpmEstablishedCallOutId = 0;
UINT64 g_uEstablishedFilterId = 0;

UINT32	g_uFwpsStreamCallOutId = 0;
UINT32	g_uFwpmStreamCallOutId = 0;
UINT64 g_uStreamFilterId = 0;

HANDLE	g_hEngine = NULL;

NTSTATUS WfpDriverEntry(__in struct _DRIVER_OBJECT* DriverObject, __in PUNICODE_STRING RegistryPath)
{
	DbgBreakPoint();
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	UNREFERENCED_PARAMETER(RegistryPath);
	do
	{
		if (DriverObject == NULL)
		{
			break;
		}
		DriverObject->DriverUnload = WfpDriverUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = WfpSampleIRPDispatch;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = WfpSampleIRPDispatch;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WfpSampleIRPDispatch;
		if (FALSE == InitRuleInfo())
		{
			break;
		}
		g_pDeviceObj = CreateDevice(DriverObject);
		if (g_pDeviceObj == NULL)
		{
			break;
		}
		if (InitWfp() != STATUS_SUCCESS)
		{
			break;
		}
		nStatus = STATUS_SUCCESS;
	} while (FALSE);
	if (nStatus != STATUS_SUCCESS)
	{
		UninitWfp();
		DeleteDevice();
		UninitRuleInfo();
	}
	return nStatus;
}

NTSTATUS WfpSampleIRPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	ULONG	ulInformation = 0;
	UNREFERENCED_PARAMETER(DeviceObject);
	do
	{
		PIO_STACK_LOCATION	IrpStack = NULL;
		PVOID pSystemBuffer = NULL;
		ULONG uInLen = 0;
		if (Irp == NULL)
		{
			break;
		}
		pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
		IrpStack = IoGetCurrentIrpStackLocation(Irp);
		if (IrpStack == NULL)
		{
			break;
		}
		uInLen = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
		if (IrpStack->MajorFunction != IRP_MJ_DEVICE_CONTROL)
		{
			break;
		}
		// 开始处理DeivceIoControl的情况
		switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_WFP_SAMPLE_ADD_RULE:
		{
			//DbgBreakPoint();
			KdPrintEx((77, 0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__));
			BOOLEAN bSucc = FALSE;
			bSucc = AddNetRuleInfo(pSystemBuffer, uInLen);
			if (bSucc == FALSE)
			{
				nStatus = STATUS_UNSUCCESSFUL;
			}
			break;
		}
		case IOCTL_WFP_SAMPLE_ADDRESS_ADD:
		{
			//DbgBreakPoint();
			KdPrintEx((77, 0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__));
			BOOLEAN bSucc = FALSE;
			bSucc = AddNetRuleInfo(pSystemBuffer, uInLen);
			if (bSucc == FALSE)
			{
				nStatus = STATUS_UNSUCCESSFUL;
			}
			break;
		}
		case IOCTL_WFP_SAMPLE_REMOVE_RULE:
		{
			//DbgBreakPoint();
			BOOLEAN bSucc = FALSE;
			bSucc = UninitRuleInfo();
			if (bSucc == FALSE)
			{
				nStatus = STATUS_UNSUCCESSFUL;
			}
			break;
		}
		default:
		{
			ulInformation = 0;
			nStatus = STATUS_UNSUCCESSFUL;
		}
		}
	} while (FALSE);
	if (Irp != NULL)
	{
		Irp->IoStatus.Information = ulInformation;
		Irp->IoStatus.Status = nStatus;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return nStatus;
}

PDEVICE_OBJECT	CreateDevice(__in struct _DRIVER_OBJECT* DriverObject)
{
	UNICODE_STRING	uDeviceName = { 0 };
	UNICODE_STRING	uSymbolName = { 0 };
	PDEVICE_OBJECT	pDeviceObj = NULL;
	NTSTATUS nStatsus = STATUS_UNSUCCESSFUL;
	RtlInitUnicodeString(&uDeviceName, WFP_DEVICE_NAME);
	RtlInitUnicodeString(&uSymbolName, WFP_SYM_LINK_NAME);
	nStatsus = IoCreateDevice(DriverObject, 0, &uDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObj);
	if (pDeviceObj != NULL)
	{
		pDeviceObj->Flags |= DO_BUFFERED_IO;
	}
	IoCreateSymbolicLink(&uSymbolName, &uDeviceName);
	return pDeviceObj;
}

VOID DeleteDevice()
{
	UNICODE_STRING uSymbolName = { 0 };
	RtlInitUnicodeString(&uSymbolName, WFP_SYM_LINK_NAME);
	IoDeleteSymbolicLink(&uSymbolName);
	if (g_pDeviceObj != NULL)
	{
		IoDeleteDevice(g_pDeviceObj);
	}
	g_pDeviceObj = NULL;
}

NTSTATUS InitWfp()
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	do
	{
		g_hEngine = OpenEngine();
		if (g_hEngine == NULL)
		{
			break;
		}
		//注册呼出接口
		if (STATUS_SUCCESS != WfpRegisterCallouts(g_pDeviceObj))
		{
			break;
		}
		//添加呼出接口
		if (STATUS_SUCCESS != WfpAddCallouts())
		{
			break;
		}
		////添加呼出接口
		if (STATUS_SUCCESS != WfpAddCalloutsStream())
		{
			break;
		}
		//添加子层
		if (STATUS_SUCCESS != WfpAddSubLayer())
		{
			break;
		}
		////添加子层
		if (STATUS_SUCCESS != WfpAddSubLayerStream())
		{
			break;
		}
		//添加过滤引擎
		if (STATUS_SUCCESS != WfpAddFilters())
		{
			break;
		}
		//添加过滤引擎
		if (STATUS_SUCCESS != WfpAddFiltersStream())
		{
			break;
		}
		nStatus = STATUS_SUCCESS;
	} while (FALSE);
	return nStatus;
}

VOID UninitWfp()
{
	WfpRemoveFilters();
	WfpRemoveSubLayer();
	WfpRemoveCallouts();
	WfpUnRegisterCallouts();
	CloseEngine();
}

void  WfpDriverUnload(__in struct _DRIVER_OBJECT* DriverObject)
{
	UninitWfp();
	DeleteDevice();
	UninitRuleInfo();
	return;
}


HANDLE OpenEngine()
{
	FWPM_SESSION0 Session = { 0 };
	HANDLE hEngine = NULL;
	FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, &Session, &hEngine);
	return hEngine;
}

void CloseEngine()
{
	if (g_hEngine != NULL)
	{
		FwpmEngineClose(g_hEngine);
	}
	g_hEngine = NULL;
}


NTSTATUS WfpRegisterCalloutImple(
	IN OUT void* deviceObject,
	IN  FWPS_CALLOUT_CLASSIFY_FN ClassifyFunction,
	IN  FWPS_CALLOUT_NOTIFY_FN NotifyFunction,
	IN  FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN FlowDeleteFunction,
	IN  GUID const* calloutKey,
	IN  UINT32 flags,
	OUT UINT32* calloutId
)
{
	FWPS_CALLOUT sCallout;
	NTSTATUS status = STATUS_SUCCESS;

	memset(&sCallout, 0, sizeof(FWPS_CALLOUT));

	sCallout.calloutKey = *calloutKey;
	sCallout.flags = flags;
	sCallout.classifyFn = ClassifyFunction;
	sCallout.notifyFn = NotifyFunction;
	sCallout.flowDeleteFn = FlowDeleteFunction;

	status = FwpsCalloutRegister(deviceObject, &sCallout, calloutId);

	return status;
}

NTSTATUS WfpRegisterCallouts(IN OUT void* deviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do
	{
		if (deviceObject == NULL)
		{
			break;
		}
		//注册呼出接口
		status = WfpRegisterCalloutImple(deviceObject,
			Wfp_Sample_Established_ClassifyFn_V4,
			Wfp_Sample_Established_NotifyFn_V4,
			Wfp_Sample_Established_FlowDeleteFn_V4,
			&WFP_SAMPLE_ESTABLISHED_CALLOUT_V4_GUID,
			0,
			&g_uFwpsEstablishedCallOutId);
		if (status != STATUS_SUCCESS)
		{
			break;
		}
		//注册呼出接口
		status = WfpRegisterCalloutImple(deviceObject,
			Wfp_Sample_Stream_ClassifyFn_V4,
			Wfp_Sample_Established_NotifyFn_V4,
			Wfp_Sample_Established_FlowDeleteFn_V4,
			&WFP_SAMPLE_STREAM_CALLOUT_V4_GUID,
			0,
			&g_uFwpsStreamCallOutId);
		if (status != STATUS_SUCCESS)
		{
			break;
		}
		status = STATUS_SUCCESS;
	} while (FALSE);
	return status;
}

VOID WfpUnRegisterCallouts()
{
	FwpsCalloutUnregisterById(g_uFwpsEstablishedCallOutId);
	g_uFwpsEstablishedCallOutId = 0;
	FwpsCalloutUnregisterById(g_uFwpsStreamCallOutId);
	g_uFwpsStreamCallOutId = 0;
}


NTSTATUS WfpAddCallouts()
{
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_CALLOUT fwpmCallout = { 0 };
	fwpmCallout.flags = 0;
	do
	{
		if (g_hEngine == NULL)
		{
			break;
		}
		fwpmCallout.displayData.name = (wchar_t*)WFP_SAMPLE_ESTABLISHED_CALLOUT_DISPLAY_NAME;
		fwpmCallout.displayData.description = (wchar_t*)WFP_SAMPLE_ESTABLISHED_CALLOUT_DISPLAY_NAME;
		fwpmCallout.calloutKey = WFP_SAMPLE_ESTABLISHED_CALLOUT_V4_GUID;
		fwpmCallout.applicableLayer = FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
		status = FwpmCalloutAdd(g_hEngine, &fwpmCallout, NULL, &g_uFwpmEstablishedCallOutId);
		if (!NT_SUCCESS(status) && (status != STATUS_FWP_ALREADY_EXISTS))
		{
			break;
		}
		status = STATUS_SUCCESS;
	} while (FALSE);
	return status;
}
NTSTATUS WfpAddCalloutsStream()
{
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_CALLOUT fwpmCallout = { 0 };
	fwpmCallout.flags = 0;
	do
	{
		if (g_hEngine == NULL)
		{
			break;
		}
		fwpmCallout.displayData.name = (wchar_t*)WFP_SAMPLE_STREAM_CALLOUT_DISPLAY_NAME;
		fwpmCallout.displayData.description = (wchar_t*)WFP_SAMPLE_STREAM_CALLOUT_DISPLAY_NAME;
		fwpmCallout.calloutKey = WFP_SAMPLE_STREAM_CALLOUT_V4_GUID;
		fwpmCallout.applicableLayer = FWPM_LAYER_STREAM_V4;
		status = FwpmCalloutAdd(g_hEngine, &fwpmCallout, NULL, &g_uFwpmStreamCallOutId);
		if (!NT_SUCCESS(status) && (status != STATUS_FWP_ALREADY_EXISTS))
		{
			break;
		}
		status = STATUS_SUCCESS;
	} while (FALSE);
	return status;
}

VOID WfpRemoveCallouts()
{
	if (g_hEngine != NULL)
	{
		FwpmCalloutDeleteById(g_hEngine, g_uFwpmEstablishedCallOutId);
		g_uFwpmEstablishedCallOutId = 0;
		FwpmCalloutDeleteById(g_hEngine, g_uFwpmStreamCallOutId);
		g_uFwpmStreamCallOutId = 0;
	}

}

NTSTATUS WfpAddSubLayer()
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	FWPM_SUBLAYER SubLayer = { 0 };
	SubLayer.flags = 0;
	SubLayer.displayData.description = WFP_SAMPLE_SUB_LAYER_DISPLAY_NAME;
	SubLayer.displayData.name = WFP_SAMPLE_SUB_LAYER_DISPLAY_NAME;
	SubLayer.subLayerKey = WFP_SAMPLE_SUBLAYER_GUID;
	SubLayer.weight = 65535;
	if (g_hEngine != NULL)
	{
		nStatus = FwpmSubLayerAdd(g_hEngine, &SubLayer, NULL);
	}
	return nStatus;
}
NTSTATUS WfpAddSubLayerStream()
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	FWPM_SUBLAYER SubLayer = { 0 };
	SubLayer.flags = 0;
	SubLayer.displayData.description = WFP_SAMPLE_STREAM_SUB_LAYER_DISPLAY_NAME;
	SubLayer.displayData.name = WFP_SAMPLE_STREAM_SUB_LAYER_DISPLAY_NAME;
	SubLayer.subLayerKey = WFP_SAMPLE_STREAM_SUBLAYER_GUID;
	SubLayer.weight = 65535;
	if (g_hEngine != NULL)
	{
		nStatus = FwpmSubLayerAdd(g_hEngine, &SubLayer, NULL);
	}
	return nStatus;
}

VOID WfpRemoveSubLayer()
{
	if (g_hEngine != NULL)
	{
		FwpmSubLayerDeleteByKey(g_hEngine, &WFP_SAMPLE_SUBLAYER_GUID);
		FwpmSubLayerDeleteByKey(g_hEngine, &WFP_SAMPLE_STREAM_SUBLAYER_GUID);
	}
}


NTSTATUS WfpAddFilters()
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	do
	{
		FWPM_FILTER0 Filter = { 0 };
		FWPM_FILTER0 FilterNew = { 0 };
		FWPM_FILTER_CONDITION FilterCondition[1] = { 0 };
		FWP_V4_ADDR_AND_MASK AddrAndMask = { 0 };
		if (g_hEngine == NULL)
		{
			break;
		}
		Filter.displayData.description = WFP_SAMPLE_FILTER_ESTABLISH_DISPLAY_NAME;
		Filter.displayData.name = WFP_SAMPLE_FILTER_ESTABLISH_DISPLAY_NAME;
		Filter.flags = 0;
		Filter.layerKey = FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
		Filter.subLayerKey = WFP_SAMPLE_SUBLAYER_GUID;
		Filter.weight.type = FWP_EMPTY;
		Filter.numFilterConditions = 1;
		Filter.filterCondition = FilterCondition;
		Filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		Filter.action.calloutKey = WFP_SAMPLE_ESTABLISHED_CALLOUT_V4_GUID;
		FilterCondition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		FilterCondition[0].matchType = FWP_MATCH_EQUAL;
		FilterCondition[0].conditionValue.type = FWP_V4_ADDR_MASK;//要检测的类型
		FilterCondition[0].conditionValue.v4AddrMask = &AddrAndMask;//要检测的值
		nStatus = FwpmFilterAdd(g_hEngine, &Filter, NULL, &g_uEstablishedFilterId);
		if (STATUS_SUCCESS != nStatus)
		{
			break;
		}
		nStatus = STATUS_SUCCESS;
	} while (FALSE);
	return nStatus;
}
NTSTATUS WfpAddFiltersStream()
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	do
	{
		FWPM_FILTER0 Filter = { 0 };
		FWPM_FILTER0 FilterNew = { 0 };
		FWPM_FILTER_CONDITION FilterCondition[1] = { 0 };
		FWP_V4_ADDR_AND_MASK AddrAndMask = { 0 };
		if (g_hEngine == NULL)
		{
			break;
		}
		Filter.displayData.description = WFP_SAMPLE_FILTER_STREAM_DISPLAY_NAME;
		Filter.displayData.name = WFP_SAMPLE_FILTER_STREAM_DISPLAY_NAME;
		Filter.flags = 0;
		Filter.layerKey = FWPM_LAYER_STREAM_V4;
		Filter.subLayerKey = WFP_SAMPLE_STREAM_SUBLAYER_GUID;
		Filter.weight.type = FWP_EMPTY;
		Filter.numFilterConditions = 1;
		Filter.filterCondition = FilterCondition;
		Filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		Filter.action.calloutKey = WFP_SAMPLE_STREAM_CALLOUT_V4_GUID;
		FilterCondition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		FilterCondition[0].matchType = FWP_MATCH_EQUAL;
		FilterCondition[0].conditionValue.type = FWP_V4_ADDR_MASK;//要检测的类型
		FilterCondition[0].conditionValue.v4AddrMask = &AddrAndMask;//要检测的值
		nStatus = FwpmFilterAdd(g_hEngine, &Filter, NULL, &g_uStreamFilterId);
		if (STATUS_SUCCESS != nStatus)
		{
			break;
		}
		nStatus = STATUS_SUCCESS;
	} while (FALSE);
	return nStatus;
}

VOID WfpRemoveFilters()
{
	if (g_hEngine != NULL)
	{
		FwpmFilterDeleteById(g_hEngine, g_uEstablishedFilterId);
		FwpmFilterDeleteById(g_hEngine, g_uStreamFilterId);
	}
}

VOID NTAPI Wfp_Sample_Established_ClassifyFn_V4(
	IN const FWPS_INCOMING_VALUES0* inFixedValues,
	IN const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	IN OUT VOID* layerData,
	IN OPTIONAL const void* classifyContext,
	IN const FWPS_FILTER3* filter,
	IN UINT64  flowContext,
	OUT FWPS_CLASSIFY_OUT0* classifyOut
)


{
	//DbgBreakPoint();
	WORD	wDirection = 0;
	WORD	wRemotePort = 0;
	WORD	wSrcPort = 0;
	WORD	wProtocol = 0;
	ULONG	ulSrcIPAddress = 0;
	ULONG	ulRemoteIPAddress = 0;
	if (!(classifyOut->rights & FWPS_RIGHT_ACTION_WRITE))
	{
		return;
	}
	//wDirection表示数据包的方向,取值为	//FWP_DIRECTION_INBOUND/FWP_DIRECTION_OUTBOUND
	wDirection = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_DIRECTION].value.int8;

	//wSrcPort表示本地端口，主机序
	wSrcPort = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_PORT].value.uint16;

	//wRemotePort表示远端端口，主机序
	wRemotePort = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_PORT].value.uint16;

	//ulSrcIPAddress 表示源IP
	ulSrcIPAddress = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS].value.uint32;

	//ulRemoteIPAddress 表示远端IP
	ulRemoteIPAddress = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_ADDRESS].value.uint32;
	DbgPrint("remote ip   is %x\n", ulRemoteIPAddress);
	DbgPrint("remote port is %x\n", wRemotePort);

	//wProtocol表示网络协议，可以取值是IPPROTO_ICMP/IPPROTO_UDP/IPPROTO_TCP
	wProtocol = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL].value.uint8;

	//默认"允许"(PERMIT)
	classifyOut->actionType = FWP_ACTION_PERMIT;


	if (IsHitRule(ulRemoteIPAddress))
	{
		classifyOut->actionType = FWP_ACTION_BLOCK;
	}
	if (IsHitRulePort(wRemotePort))
	{
		classifyOut->actionType = FWP_ACTION_BLOCK;
	}

	//清除FWPS_RIGHT_ACTION_WRITE标记
	if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
	{
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
	}
	return;
}

VOID NTAPI Wfp_Sample_Stream_ClassifyFn_V4(
	IN const FWPS_INCOMING_VALUES0* inFixedValues,
	IN const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
	IN OUT VOID* layerData,
	IN OPTIONAL const void* classifyContext,
	IN const FWPS_FILTER3* filter,
	IN UINT64  flowContext,
	OUT FWPS_CLASSIFY_OUT0* classifyOut
)


{
	// FWPM_LAYER_STREAM_V4
	// DbgBreakPoint();
	// 默认"允许"(PERMIT)
	classifyOut->actionType = FWP_ACTION_PERMIT;

	FWPS_STREAM_CALLOUT_IO_PACKET* streamPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*)layerData;
	if (streamPacket != NULL && streamPacket->streamData != NULL &&
		streamPacket->streamData->dataLength != 0)
	{
		//DbgBreakPoint();
		////得到数据流指针
		FWPS_STREAM_DATA0* streamBuffer = streamPacket->streamData;

		BYTE* stream = NULL;
#define TAG_NAME_NOTIFY 'oNnM'
		SIZE_T streamLength = streamBuffer->dataLength;
		SIZE_T bytesCopied = 0;
		stream = (BYTE*)ExAllocatePoolWithTag(NonPagedPool,
			streamLength + 4,
			TAG_NAME_NOTIFY);
		RtlZeroMemory(stream, streamLength + 4);
		FwpsCopyStreamDataToBuffer0(
			streamBuffer,
			stream,
			streamLength,
			&bytesCopied);
		// 检查 streamData 是否为 NULL
		PCHAR visibleString = ProcessVisibleBytes(stream, streamLength);
		KdPrintEx((77, 0, "%s %s %d %d %s\n", __FILE__, __FUNCTION__, __LINE__, streamLength, visibleString));
		if (visibleString != NULL) {
			if (IsHitRuleUrl(visibleString)) {
				classifyOut->actionType = FWP_ACTION_BLOCK;
			}
			ExFreePoolWithTag(visibleString, L"ProcessVisibleBytes");
		}
	}

	//清除FWPS_RIGHT_ACTION_WRITE标记
	if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
	{
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
	}
	return;
}



NTSTATUS NTAPI Wfp_Sample_Established_NotifyFn_V4(
	IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
	IN  const GUID* filterKey,
	IN  const FWPS_FILTER* filter)
{
	return STATUS_SUCCESS;
}

VOID NTAPI Wfp_Sample_Established_FlowDeleteFn_V4(
	IN UINT16  layerId,
	IN UINT32  calloutId,
	IN UINT64  flowContext
)
{

}
