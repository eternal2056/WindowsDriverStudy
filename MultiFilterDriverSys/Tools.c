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

PCHAR ProcessVisibleBytes(PVOID data, SIZE_T dataSize)
{
	if (data == NULL || dataSize == 0)
	{
		// 处理无效输入
		return NULL;
	}

	// 转换指针类型，方便按字节访问
	UCHAR* byteData = (UCHAR*)data;

	// 为存储可见字符的字符串分配内存
	SIZE_T visibleStringLength = 0;
	for (SIZE_T i = 0; i < dataSize; i++)
	{
		// 检查是否为可见字符
		if (byteData[i] >= 0x20 && byteData[i] <= 0x7E)
		{
			visibleStringLength++;
		}
	}

	if (visibleStringLength == 0)
	{
		// 没有可见字符
		return NULL;
	}

	// 分配内存
	PCHAR visibleString = ExAllocatePoolWithTag(NonPagedPool, visibleStringLength + 1, L'ProcessVisibleBytes');
	if (visibleString == NULL)
	{
		// 内存分配失败
		return NULL;
	}

	// 构建可见字符字符串
	SIZE_T index = 0;
	for (SIZE_T i = 0; i < dataSize; i++)
	{
		// 检查是否为可见字符
		if (byteData[i] >= 0x20 && byteData[i] <= 0x7E)
		{
			visibleString[index++] = (CHAR)byteData[i];
		}
	}

	// 在字符串末尾添加 null 终止符
	visibleString[index] = '\0';

	return visibleString;
}