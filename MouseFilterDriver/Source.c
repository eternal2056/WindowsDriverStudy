#include <ntddk.h>
#include <ntddmou.h>

#ifndef _MOUSE_FILTER_H
#define _MOUSE_FILTER_H

#define DEVICE_NAME          L"\\Device\\MouseFilter"
#define DOS_DEVICE_NAME  L"\\DosDevices\\MouseFilter"

#define POOL_TAG               'tsiL'

typedef NTSTATUS(*READ_DISPATCH)(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

typedef struct _DEVICE_EXTENSION {

	PDEVICE_OBJECT  pDeviceObject;

} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef struct _MOUSE_FILTER_DATA {

	PDRIVER_OBJECT  pMouseDriverObject; // "\\Driver\\Mouclass" 的指针
	READ_DISPATCH OldRead; // 原来的IRP_MJ_READ 派遣例程
	ULONG LeftDownTime; // 上次鼠标左键按下的时刻 
	KMUTEX ReadMutex; // Read互斥体
	SINGLE_LIST_ENTRY ListHead;// 保存Pending IRP的链表
	SINGLE_LIST_ENTRY CancelHead;// 用来对上面链表的进行复制

}MOUSE_FILTER_DATA, * PMOUSE_FILTER_DATA;

typedef struct _PENDING_IRP_LIST {

	SINGLE_LIST_ENTRY SingleListEntry;
	PIRP PendingIrp;

}PENDING_IRP_LIST, * PPENDING_IRP_LIST;

extern POBJECT_TYPE* IoDriverObjectType;

extern NTSTATUS ObReferenceObjectByName(
	PUNICODE_STRING ObjectName,
	ULONG Attributes,
	PACCESS_STATE AccessState,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType,
	KPROCESSOR_MODE AccessMode,
	PVOID ParseContext,
	PVOID* Object
);


#endif // #ifndef _MOUSE_FILTER_H



MOUSE_FILTER_DATA gFilterData = { 0 };


NTSTATUS
CreateClose(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
)
{
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

	UNREFERENCED_PARAMETER(DeviceObject);

	PAGED_CODE();

	KdPrint(("Entered IRP_MJ_%s\n", (irpStack->MajorFunction == IRP_MJ_CREATE) ? "CREATE" : "CLOSE"));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Irp->IoStatus.Status;
}

NTSTATUS
DeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	PAGED_CODE();

	Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Irp->IoStatus.Status;
}

VOID CleanUP(__in PDEVICE_OBJECT DeviceObject)
{
	UNICODE_STRING  SymbolicLinkName;

	if (!DeviceObject)
		return;

	// 删除符号链接和设备对象
	RtlInitUnicodeString(&SymbolicLinkName, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&SymbolicLinkName);
	IoDeleteDevice(DeviceObject);

	//恢复IRP  hook
	if (gFilterData.OldRead)
	{
		InterlockedExchange(
			(PLONG)&gFilterData.pMouseDriverObject->MajorFunction[IRP_MJ_READ],
			(LONG)gFilterData.OldRead
		);
	}

	if (gFilterData.pMouseDriverObject)
	{
		ObDereferenceObject(gFilterData.pMouseDriverObject);
	}
}

// 取消等待的IRP
VOID CancelPendingIrp()
{
	PPENDING_IRP_LIST PendingList = NULL, CancelList = NULL;
	PSINGLE_LIST_ENTRY pSingleListEntry = NULL;

	// 获取互斥体，保护链表gFilterData.ListHead
	KeWaitForMutexObject(&gFilterData.ReadMutex, Executive, KernelMode, FALSE, NULL);

	pSingleListEntry = gFilterData.ListHead.Next;
	while (pSingleListEntry)
	{
		PendingList = CONTAINING_RECORD(pSingleListEntry, PENDING_IRP_LIST, SingleListEntry);
		KdPrint(("Copy Single List = 0x%x", PendingList));

		// 复制链表，然后将取消IRP的操作放到新的链表中处理
		CancelList = (PPENDING_IRP_LIST)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDING_IRP_LIST), POOL_TAG);
		if (CancelList)
		{
			RtlCopyMemory(CancelList, PendingList, sizeof(PENDING_IRP_LIST));
			PushEntryList(&gFilterData.CancelHead, &CancelList->SingleListEntry);
		}

		pSingleListEntry = pSingleListEntry->Next;
	}
	// 释放互斥体
	KeReleaseMutex(&gFilterData.ReadMutex, FALSE);


	// 之所以要复制一个新的链表来取消IRP (通过调用IoCancelIrp)，
	// 是因为IoCancelIrp 会调用MyReadComplete 这个完成例程回调，
	// 而MyReadComplete里面又对链表进行操作，这样会破坏链表的结构
	pSingleListEntry = PopEntryList(&gFilterData.CancelHead);
	while (pSingleListEntry)
	{
		CancelList = CONTAINING_RECORD(pSingleListEntry, PENDING_IRP_LIST, SingleListEntry);
		if (CancelList)
		{
			KdPrint(("CancelPendingIrp = 0x%x", CancelList->PendingIrp));

			if (!CancelList->PendingIrp->Cancel)
			{// 在这里，取出复制链表中的IRP，然后进行取消
				IoCancelIrp(CancelList->PendingIrp);
			}
			ExFreePoolWithTag(CancelList, POOL_TAG);
			pSingleListEntry = PopEntryList(&gFilterData.CancelHead);
		}
	}
}


VOID
Unload(
	IN PDRIVER_OBJECT DriverObject
)
{
	PAGED_CODE();

	KdPrint(("Entered Unload\n"));

	CleanUP(DriverObject->DeviceObject);

	CancelPendingIrp();

	//等待所有的IRP 完成    
	while (gFilterData.ListHead.Next)
	{
		LARGE_INTEGER lDelay;
		lDelay.QuadPart = -10 * 1000 * 1000; // 1秒
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}
}

//判断是否是假双击，根据时间间隔判断
BOOLEAN IsFakeDoubleClick()
{
	LARGE_INTEGER CurrentTime;
	BOOLEAN Flag = FALSE;

	KeQueryTickCount(&CurrentTime);

	CurrentTime.QuadPart *= KeQueryTimeIncrement();
	CurrentTime.QuadPart /= 10000;

	//两次点击时间间隔小于100ms，视为假双击
	if (CurrentTime.LowPart - gFilterData.LeftDownTime < 100)
	{
		Flag = TRUE;
	}

	InterlockedExchange(&gFilterData.LeftDownTime, CurrentTime.LowPart);
	return Flag;
}

//IRP_MJ_READ 完成函数
NTSTATUS MyReadComplete(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)
{
	ULONG i, num;
	PMOUSE_INPUT_DATA data;
	PSINGLE_LIST_ENTRY pSingleListEntry = &gFilterData.ListHead;
	PPENDING_IRP_LIST PendingList = NULL;

	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		data = Irp->AssociatedIrp.SystemBuffer;
		num = Irp->IoStatus.Information / sizeof(MOUSE_INPUT_DATA);

		for (i = 0; i < num; i++)
		{
			if (data[i].ButtonFlags & MOUSE_LEFT_BUTTON_DOWN)
			{ // 鼠标左键按下
				if (IsFakeDoubleClick())
				{   //假双击
					KdPrint(("Got a Fake Double CLick!!\r\n"));
					data[i].ButtonFlags = 0; //忽略掉
				}
				else
				{
					//KdPrint(("Left Button Down"));
				}
			}
		}
	}

	KeWaitForMutexObject(&gFilterData.ReadMutex, Executive, KernelMode, FALSE, NULL);

	for (; pSingleListEntry->Next; pSingleListEntry = pSingleListEntry->Next)
	{
		PendingList = CONTAINING_RECORD(pSingleListEntry->Next, PENDING_IRP_LIST, SingleListEntry);

		if (PendingList->PendingIrp == Irp)
		{// 从链表中移除
			pSingleListEntry->Next = pSingleListEntry->Next->Next;
			ExFreePoolWithTag(PendingList, POOL_TAG);
			break;
		}
	}

	KeReleaseMutex(&gFilterData.ReadMutex, FALSE);

	//KdPrint(("MyReadComplete Irp= 0x%X, 0x%X", Irp,  Irp->IoStatus.Status));    

	//调用原来的完成函数
	if ((Irp->StackCount > 1) && (Context != NULL))
	{
		return ((PIO_COMPLETION_ROUTINE)Context)(DeviceObject, Irp, NULL);
	}
	else
	{
		return Irp->IoStatus.Status;
	}
}


NTSTATUS
MyRead(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
)
{
	PPENDING_IRP_LIST PendingList = NULL;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

	//KdPrint(("MyRead Irp= 0x%p ", Irp)); 

	PendingList = (PPENDING_IRP_LIST)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDING_IRP_LIST), POOL_TAG);

	if (PendingList)
	{
		PendingList->PendingIrp = Irp;

		KeWaitForMutexObject(&gFilterData.ReadMutex, Executive, KernelMode, FALSE, NULL);
		PushEntryList(&gFilterData.ListHead, &PendingList->SingleListEntry);
		KeReleaseMutex(&gFilterData.ReadMutex, FALSE);

		// 按理说，应该调用IoSetCompletionRoutine来设置完成例程的
		// 但是，那样会导致完成例程根本不会被执行，Why?
		irpSp->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

		//保留原来的完成函数
		irpSp->Context = irpSp->CompletionRoutine;
		irpSp->CompletionRoutine = (PIO_COMPLETION_ROUTINE)MyReadComplete;
	}

	return  gFilterData.OldRead(DeviceObject, Irp);
}

NTSTATUS
DriverEntry(
	IN  PDRIVER_OBJECT  DriverObject,
	IN  PUNICODE_STRING RegistryPath
)
{
	NTSTATUS  status = STATUS_SUCCESS;
	UNICODE_STRING  DeviceName;
	UNICODE_STRING  SymbolicLinkName;
	UNICODE_STRING  MouseDriver;
	PDEVICE_OBJECT  DeviceObject = NULL;
	PDEVICE_EXTENSION   deviceExtension = NULL;

	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint(("Entered DriverEntry\n"));

	KeInitializeMutex(&gFilterData.ReadMutex, 0);
	gFilterData.ListHead.Next = NULL;
	gFilterData.CancelHead.Next = NULL;

	DriverObject->MajorFunction[IRP_MJ_CREATE] =
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
	DriverObject->DriverUnload = Unload;

	//创建设备对象
	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
	status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),
		&DeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&DeviceObject
	);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateDevice failed, 0x%0x!\n", status));
		return status;
	}

	//创建符号链接
	RtlInitUnicodeString(&SymbolicLinkName, DOS_DEVICE_NAME);
	status = IoCreateSymbolicLink(&SymbolicLinkName, &SymbolicLinkName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateSymbolicLink failed, 0x%0x!\n", status));
		CleanUP(DeviceObject);
		return status;
	}

	deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

	deviceExtension->pDeviceObject = DeviceObject;

	//DeviceObject->Flags |= DO_BUFFERED_IO;
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


	//取得鼠标驱动对象
	RtlInitUnicodeString(&MouseDriver, L"\\Driver\\Mouclass");
	status = ObReferenceObjectByName(
		&MouseDriver,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		&gFilterData.pMouseDriverObject
	);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("Get mouse object failed, 0x%0x\n", status));
		CleanUP(DeviceObject);
		return status;
	}


	//保存原来的IRP_MJ_READ 派遣例程
	gFilterData.OldRead = gFilterData.pMouseDriverObject->MajorFunction[IRP_MJ_READ];

	if (gFilterData.OldRead) // IRP HOOK
	{
		InterlockedExchange(
			(PLONG)&gFilterData.pMouseDriverObject->MajorFunction[IRP_MJ_READ],
			(LONG)MyRead
		);
	}
	else
	{
		CleanUP(DeviceObject);
		return STATUS_UNSUCCESSFUL;
	}

	return status;
}


