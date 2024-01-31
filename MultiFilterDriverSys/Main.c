#include "Main.h"


void MainDriverUnload(__in struct _DRIVER_OBJECT* DriverObject)
{
	WfpDriverUnload(DriverObject);
	NPUnload(DriverObject);
	CoverProcessDriverUnload(DriverObject);
	return;
}

NTSTATUS IrpCreateDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	NTSTATUS result = STATUS_SUCCESS;
	UNICODE_STRING DeviceName = GetDeviceObjectName(DeviceObject);
	//UNICODE_STRING WfpDeviceNameString;
	//RtlInitUnicodeString(&WfpDeviceNameString, WFP_DEVICE_NAME);
	KdPrint(("File Name: %wZ\n", &DeviceName));
	result = RtlCompareUnicodeString(&DeviceName, &WFP_DEVICE_NAME, TRUE);
	if (NT_SUCCESS(result))
	{
		WfpSampleIRPDispatch(DeviceObject, Irp);
	}
	result = RtlCompareUnicodeString(&DeviceName, &KILLRULE_NTDEVICE_NAME, TRUE);
	if (NT_SUCCESS(result))
	{
		CreateFileDevice(DeviceObject, Irp);
	}

	return nStatus;
}
NTSTATUS IrpCloseDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	NTSTATUS result = STATUS_SUCCESS;
	UNICODE_STRING DeviceName = GetDeviceObjectName(DeviceObject);
	//UNICODE_STRING WfpDeviceNameString;
	//RtlInitUnicodeString(&WfpDeviceNameString, WFP_DEVICE_NAME);
	KdPrint(("File Name: %wZ\n", &DeviceName));
	result = RtlCompareUnicodeString(&DeviceName, &WFP_DEVICE_NAME, TRUE);
	if (NT_SUCCESS(result))
	{
		WfpSampleIRPDispatch(DeviceObject, Irp);
	}
	result = RtlCompareUnicodeString(&DeviceName, &KILLRULE_NTDEVICE_NAME, TRUE);
	if (NT_SUCCESS(result))
	{
		CloseFileDevice(DeviceObject, Irp);
	}

	return nStatus;
}
NTSTATUS IrpControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	NTSTATUS result = STATUS_SUCCESS;
	UNICODE_STRING DeviceName = GetDeviceObjectName(DeviceObject);
	//UNICODE_STRING WfpDeviceNameString;
	//RtlInitUnicodeString(&WfpDeviceNameString, WFP_DEVICE_NAME);
	KdPrint(("File Name: %wZ\n", &DeviceName));
	result = RtlCompareUnicodeString(&DeviceName, &WFP_DEVICE_NAME, TRUE);
	if (NT_SUCCESS(result))
	{
		WfpSampleIRPDispatch(DeviceObject, Irp);
	}
	result = RtlCompareUnicodeString(&DeviceName, &KILLRULE_NTDEVICE_NAME, TRUE);
	if (NT_SUCCESS(result))
	{
		MainDispatcher(DeviceObject, Irp);
	}

	return nStatus;
}

NTSTATUS DriverEntry(__in struct _DRIVER_OBJECT* DriverObject, __in PUNICODE_STRING RegistryPath)
{
	DbgBreakPoint();
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	UNREFERENCED_PARAMETER(RegistryPath);
	if (DriverObject == NULL)
	{
		return nStatus;
	}
	nStatus = WfpDriverEntry(DriverObject, RegistryPath);
	nStatus = MiniFilterDriverEntry(DriverObject, RegistryPath);
	nStatus = CoverProcessDriverEntry(DriverObject, RegistryPath);
	DriverObject->DriverUnload = MainDriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreateDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpCreateDispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpControlDispatch;
	return nStatus;
}

//NTSTATUS DriverEntry(__in struct _DRIVER_OBJECT* DriverObject, __in PUNICODE_STRING RegistryPath)
//{
//	DbgBreakPoint();
//	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
//	nStatus = WfpDriverEntry(DriverObject, RegistryPath);
//	nStatus = MiniFilterDriverEntry(DriverObject, RegistryPath);
//	nStatus = CoverProcessDriverEntry(DriverObject, RegistryPath);
//
//	return nStatus;
//}