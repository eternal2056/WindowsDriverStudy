#include "Tools.h"
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