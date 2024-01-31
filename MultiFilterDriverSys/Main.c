#include "Main.h"

UNICODE_STRING GetDeviceObjectName(PDEVICE_OBJECT deviceObject)
{
	NTSTATUS status;
	OBJECT_NAME_INFORMATION* objectNameInfo;
	ULONG returnLength;

	// 调用之前确保 IRQL 已降至 PASSIVE_LEVEL 或者低于

	// 获取所需缓冲区大小
	status = ObQueryNameString(deviceObject, NULL, 0, &returnLength);
	if (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		// 分配缓冲区
		objectNameInfo = (OBJECT_NAME_INFORMATION*)ExAllocatePoolWithTag(
			NonPagedPool,
			returnLength,
			L'GetDeviceObjectName'
		);

		if (objectNameInfo != NULL)
		{
			// 再次调用获取对象名称
			status = ObQueryNameString(deviceObject, objectNameInfo, returnLength, &returnLength);

			if (NT_SUCCESS(status))
			{
				// objectNameInfo->Name 包含了文件对象的名称
				KdPrint(("File Name: %wZ\n", &objectNameInfo->Name));
				return objectNameInfo->Name;
			}

			// 释放缓冲区
			ExFreePoolWithTag(objectNameInfo, L'GetDeviceObjectName');
		}
		else
		{
			KdPrint(("Failed to allocate memory\n"));
		}
	}
	else
	{
		KdPrint(("Failed to query object name length\n"));
	}
}
void MainDriverUnload(__in struct _DRIVER_OBJECT* DriverObject)
{
	WfpDriverUnload(DriverObject);
	CoverProcessDriverUnload(DriverObject);
	return;
}

NTSTATUS IrpCreateDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	UNICODE_STRING DeviceName = GetDeviceObjectName(DeviceObject);
	KdPrint(("File Name: %wZ\n", &DeviceName));
	return nStatus;
}
NTSTATUS IrpControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	UNICODE_STRING DeviceName = GetDeviceObjectName(DeviceObject);
	KdPrint(("File Name: %wZ\n", &DeviceName));
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
	WfpDriverEntry(DriverObject, RegistryPath);
	MiniFilterDriverEntry(DriverObject, RegistryPath);
	CoverProcessDriverEntry(DriverObject, RegistryPath);
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