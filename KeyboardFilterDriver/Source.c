#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>
#include <ntddkbd.h>

#define KILLRULE_NTDEVICE_NAME L"\\Device\\KillRuleDrv"
PDEVICE_OBJECT deviceObject = NULL;

typedef struct _MY_EXTENSION {
	PDEVICE_OBJECT lowerDeviceObject;
}MY_EXTENSION;


ULONG PendingCount = 0;

VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT* DriverObject
) {
	IoDetachDevice(((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject);
	IoDeleteDevice(deviceObject);
	LARGE_INTEGER lDelay = { 0 };
	lDelay.QuadPart = -10 * 1000 * 1000;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnload PendingCount is %d\n", PendingCount));
	while (PendingCount) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "KeDelayExecutionThread PendingCount is %d\n", PendingCount));
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnload.\n"));
}

NTSTATUS IrpPass(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoCallDriver(((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject, Irp);
	return STATUS_SUCCESS;

}

NTSTATUS MyCompletionRoutine(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
) {
	IoGetCurrentIrpStackLocation(Irp);
	// https://learn.microsoft.com/en-us/windows/win32/api/ntddkbd/ns-ntddkbd-keyboard_input_data
	KEYBOARD_INPUT_DATA* data = (KEYBOARD_INPUT_DATA*)Irp->AssociatedIrp.SystemBuffer;
	int structNum = Irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

	if (Irp->IoStatus.Status == STATUS_SUCCESS) {
		for (int i = 0; i < structNum; i++) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Preesed key is %d\n", data[i].MakeCode));
		}
	}

	if (Irp->PendingReturned) {
		IoMarkIrpPending(Irp);
	}
	PendingCount--;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "PendingCount is %d\n", PendingCount));
	return Irp->IoStatus.Status;
}

NTSTATUS ReadFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp, MyCompletionRoutine, NULL, TRUE, TRUE, TRUE);
	PendingCount++;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "PendingCount is %d\n", PendingCount));

	NTSTATUS status = IoCallDriver(((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject, Irp);
	return status;
}

NTSTATUS AttachToDevice(PDEVICE_OBJECT SourceDevice)
{
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AttachToDevice is start\n"));
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING KbdDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	status = IoAttachDevice(deviceObject, &KbdDeviceName, &((MY_EXTENSION*)deviceObject->DeviceExtension)->lowerDeviceObject);
	if (NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AttachToDevice is success\n"));
	}
	else {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "AttachToDevice is failed\n"));
	}
	return status;
}

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	//MiProcessLoaderEntry = (pMiProcessLoaderEntry)0xfffff80534b88ee4; // 这是用 dp 来定位的
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverEntry\n"));
	//KdBreakPoint();

	UNREFERENCED_PARAMETER(RegistryPath);

	UNICODE_STRING deviceName;
	UNICODE_STRING symbolicLinkName;

	RtlInitUnicodeString(&deviceName, KILLRULE_NTDEVICE_NAME);

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = IrpPass; // 其他的照常
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_READ] = ReadFileDevice; // 要读键盘[设备] 端口的值

	NTSTATUS status = IoCreateDevice(
		DriverObject,
		sizeof(MY_EXTENSION), // 因为要用到，所以要给
		&deviceName,
		FILE_DEVICE_KEYBOARD,
		0,
		TRUE,
		&deviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create device (0x%X)\n", status));
		return status;
	}

	status = AttachToDevice(deviceObject);
	if (NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to attach device (0x%X)\n", status));
	}
	else {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to attach device (0x%X)\n", status));
		IoDeleteDevice(deviceObject);
		return status;
	}

	deviceObject->Flags |= DO_BUFFERED_IO;

	status = STATUS_SUCCESS;

	return status;
}