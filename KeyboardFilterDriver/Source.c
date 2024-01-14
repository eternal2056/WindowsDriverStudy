#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>

#define KILLRULE_NTDEVICE_NAME L"\\Device\\KillRuleDrv"
PDEVICE_OBJECT deviceObject = NULL;

VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT* DriverObject
) {
	IoDeleteDevice(deviceObject);
}

NTSTATUS IrpPass(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {

}
NTSTATUS ReadFileDevice(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {

}

typedef struct _MY_EXTENSION {
	PDEVICE_OBJECT lowerDeviceObject;
}MY_EXTENSION;

NTSTATUS AttachToDevice(PDEVICE_OBJECT SourceDevice)
{
	NTSTATUS status = STATUS_SUCCESS;

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
		DriverObject->MajorFunction[i] = IrpPass;
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_READ] = ReadFileDevice;

	NTSTATUS status = IoCreateDevice(
		DriverObject,
		sizeof(MY_EXTENSION),
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