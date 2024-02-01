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
	UNICODE_STRING WfpDeviceNameString;
	UNICODE_STRING CoverProcessDeviceNameString;
	RtlInitUnicodeString(&WfpDeviceNameString, WFP_DEVICE_NAME);
	RtlInitUnicodeString(&CoverProcessDeviceNameString, KILLRULE_NTDEVICE_NAME);
	KdPrintEx((77, 0, "%s %s %d %wZ\n", __FILE__, __FUNCTION__, __LINE__, &DeviceName));
	KdPrintEx((77, 0, "%s %s %d %wZ\n", __FILE__, __FUNCTION__, __LINE__, &WfpDeviceNameString));
	KdPrintEx((77, 0, "%s %s %d %wZ\n", __FILE__, __FUNCTION__, __LINE__, &CoverProcessDeviceNameString));
	result = RtlEqualUnicodeString(&DeviceName, &WfpDeviceNameString, TRUE);
	if (result)
	{
		WfpSampleIRPDispatch(DeviceObject, Irp);
	}
	result = RtlEqualUnicodeString(&DeviceName, &CoverProcessDeviceNameString, TRUE);
	if (result)
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

	UNICODE_STRING WfpDeviceNameString;
	UNICODE_STRING CoverProcessDeviceNameString;
	RtlInitUnicodeString(&WfpDeviceNameString, WFP_DEVICE_NAME);
	RtlInitUnicodeString(&CoverProcessDeviceNameString, KILLRULE_NTDEVICE_NAME);
	KdPrintEx((77, 0, "File Name: %wZ\n", &DeviceName));
	KdPrintEx((77, 0, "File Name: %wZ\n", &WfpDeviceNameString));
	KdPrintEx((77, 0, "File Name: %wZ\n", &CoverProcessDeviceNameString));

	KdPrint(("File Name: %wZ\n", &DeviceName));
	result = RtlEqualUnicodeString(&DeviceName, &WfpDeviceNameString, TRUE);
	if (result)
	{
		WfpSampleIRPDispatch(DeviceObject, Irp);
	}
	result = RtlEqualUnicodeString(&DeviceName, &CoverProcessDeviceNameString, TRUE);
	if (result)
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

	UNICODE_STRING WfpDeviceNameString;
	UNICODE_STRING CoverProcessDeviceNameString;
	RtlInitUnicodeString(&WfpDeviceNameString, WFP_DEVICE_NAME);
	RtlInitUnicodeString(&CoverProcessDeviceNameString, KILLRULE_NTDEVICE_NAME);
	KdPrintEx((77, 0, "File Name: %wZ\n", &DeviceName));
	KdPrintEx((77, 0, "File Name: %wZ\n", &WfpDeviceNameString));
	KdPrintEx((77, 0, "File Name: %wZ\n", &CoverProcessDeviceNameString));

	KdPrintEx((77, "File Name: %wZ\n", &DeviceName));
	result = RtlEqualUnicodeString(&DeviceName, &WfpDeviceNameString, TRUE);
	if (result)
	{
		WfpSampleIRPDispatch(DeviceObject, Irp);
	}
	result = RtlEqualUnicodeString(&DeviceName, &CoverProcessDeviceNameString, TRUE);
	if (result)
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
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpCloseDispatch;
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