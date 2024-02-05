#include "ExcludeList.h"
#include "Helper.h"

#define EXCLUDE_ALLOC_TAG 'LcxE'

typedef struct _EXCULE_FILE_PATH {
	UNICODE_STRING fullPath;
	UNICODE_STRING dirName;
	UNICODE_STRING fileName;
} EXCULE_FILE_PATH, * PEXCULE_FILE_PATH;

typedef struct _EXCLUDE_FILE_LIST_ENTRY {
	LIST_ENTRY       list;
	ULONGLONG        guid;
	ULONGLONG        parentGuid;
	EXCULE_FILE_PATH path;
} EXCLUDE_FILE_LIST_ENTRY, * PEXCLUDE_FILE_LIST_ENTRY;

typedef struct _EXCLUDE_FILE_CONTEXT {
	LIST_ENTRY       listHead;
	FAST_MUTEX       listLock;
	ULONGLONG        guidCounter;
	UINT32           childCounter;
	UINT32           type;
} EXCLUDE_FILE_CONTEXT, * PEXCLUDE_FILE_CONTEXT;

BOOLEAN FillDirectoryFromPath(PEXCULE_FILE_PATH path, PUNICODE_STRING filePath);

BOOLEAN CheckExcludeListDirectory(ExcludeContext Context, PCUNICODE_STRING Path)
{
	PEXCLUDE_FILE_CONTEXT cntx = (PEXCLUDE_FILE_CONTEXT)Context;
	PEXCLUDE_FILE_LIST_ENTRY entry;
	UNICODE_STRING Directory, dir;
	BOOLEAN result = FALSE;

	Directory = *Path;
	if (Directory.Length > 0 && Directory.Buffer[Directory.Length / sizeof(WCHAR) - 1] == L'\\')
		Directory.Length -= sizeof(WCHAR);

	ExAcquireFastMutex(&cntx->listLock);

	entry = (PEXCLUDE_FILE_LIST_ENTRY)cntx->listHead.Flink;
	while (entry != (PEXCLUDE_FILE_LIST_ENTRY)&cntx->listHead)
	{
		dir = Directory;

		if (dir.Length >= entry->path.fullPath.Length)
		{
			BOOLEAN compare = TRUE;

			if (dir.Length > entry->path.fullPath.Length)
			{
				if (dir.Buffer[entry->path.fullPath.Length / sizeof(WCHAR)] != L'\\')
					compare = FALSE;
				else
					dir.Length = entry->path.fullPath.Length;
			}

			if (compare && RtlCompareUnicodeString(&entry->path.fullPath, &dir, TRUE) == 0)
			{
				result = TRUE;
				break;
			}
		}

		entry = (PEXCLUDE_FILE_LIST_ENTRY)entry->list.Flink;
	}

	ExReleaseFastMutex(&cntx->listLock);

	return result;
}

BOOLEAN CheckExcludeListDirFile(ExcludeContext Context, PCUNICODE_STRING Dir, PCUNICODE_STRING File)
{
	PEXCLUDE_FILE_CONTEXT cntx = (PEXCLUDE_FILE_CONTEXT)Context;
	PEXCLUDE_FILE_LIST_ENTRY entry;
	UNICODE_STRING Directory;
	BOOLEAN result = FALSE;

	Directory = *Dir;

	if (Directory.Length > 0 && Directory.Buffer[Directory.Length / sizeof(WCHAR) - 1] == L'\\')
		Directory.Length -= sizeof(WCHAR);

	ExAcquireFastMutex(&cntx->listLock);

	entry = (PEXCLUDE_FILE_LIST_ENTRY)cntx->listHead.Flink;
	while (entry != (PEXCLUDE_FILE_LIST_ENTRY)&cntx->listHead)
	{
		if (RtlCompareUnicodeString(&entry->path.dirName, &Directory, TRUE) == 0
			&& RtlCompareUnicodeString(&entry->path.fileName, File, TRUE) == 0)
		{
			result = TRUE;
			break;
		}

		entry = (PEXCLUDE_FILE_LIST_ENTRY)entry->list.Flink;
	}

	ExReleaseFastMutex(&cntx->listLock);

	return result;
}


NTSTATUS InitializeExcludeListContext(PExcludeContext Context, UINT32 Type)
{
	PEXCLUDE_FILE_CONTEXT cntx;

	if (Type >= ExcludeMaxType)
	{
		LogWarning("Error, invalid exclude list type: %d", Type);
		return STATUS_INVALID_MEMBER;
	}

	cntx = (PEXCLUDE_FILE_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(EXCLUDE_FILE_CONTEXT), EXCLUDE_ALLOC_TAG);
	if (!cntx)
	{
		LogWarning("Error, can't allocate memory for context: %p", Context);
		return STATUS_ACCESS_DENIED;
	}

	InitializeListHead(&cntx->listHead);
	ExInitializeFastMutex(&cntx->listLock);
	cntx->guidCounter = 1;
	cntx->childCounter = 0;
	cntx->type = Type;

	*Context = cntx;

	return STATUS_SUCCESS;
}

VOID DestroyExcludeListContext(ExcludeContext Context)
{
	PEXCLUDE_FILE_CONTEXT cntx = (PEXCLUDE_FILE_CONTEXT)Context;
	RemoveAllExcludeListEntries(Context);
	ExFreePoolWithTag(cntx, EXCLUDE_ALLOC_TAG);
}

NTSTATUS AddExcludeListFile(ExcludeContext Context, PUNICODE_STRING FilePath, PExcludeEntryId EntryId, ExcludeEntryId ParentId)
{
	return AddExcludeListEntry(Context, FilePath, ExcludeFile, EntryId, ParentId);
}

NTSTATUS AddExcludeListDirectory(ExcludeContext Context, PUNICODE_STRING DirPath, PExcludeEntryId EntryId, ExcludeEntryId ParentId)
{
	return AddExcludeListEntry(Context, DirPath, ExcludeDirectory, EntryId, ParentId);
}

NTSTATUS AddExcludeListEntry(ExcludeContext Context, PUNICODE_STRING FilePath, UINT32 Type, PExcludeEntryId EntryId, ExcludeEntryId ParentId)
{
	enum { MAX_PATH_SIZE = 1024 };
	PEXCLUDE_FILE_CONTEXT cntx = (PEXCLUDE_FILE_CONTEXT)Context;
	PEXCLUDE_FILE_LIST_ENTRY entry, head;
	UNICODE_STRING temp;
	SIZE_T size;

	if (cntx->type != Type)
	{
		LogWarning("Warning, type isn't equal: %d != %d", cntx->type, Type);
		return STATUS_INVALID_MEMBER;
	}

	if (FilePath->Length == 0 || FilePath->Length >= MAX_PATH_SIZE)
	{
		LogWarning("Warning, invalid string size : %d", (UINT32)FilePath->Length);
		return STATUS_ACCESS_DENIED;
	}

	// Allocate and fill new list entry

	size = sizeof(EXCLUDE_FILE_LIST_ENTRY) + FilePath->Length + sizeof(WCHAR);
	entry = ExAllocatePoolWithTag(NonPagedPool, size, EXCLUDE_ALLOC_TAG);
	if (entry == NULL)
	{
		LogWarning("Warning, exclude file list is not NULL : %p", cntx);
		return STATUS_ACCESS_DENIED;
	}

	RtlZeroMemory(entry, size);

	temp.Buffer = (PWCH)((PCHAR)entry + sizeof(EXCLUDE_FILE_LIST_ENTRY));
	temp.Length = 0;
	temp.MaximumLength = FilePath->Length;

	RtlCopyUnicodeString(&temp, FilePath);

	if (!FillDirectoryFromPath(&entry->path, &temp))
	{
		ExFreePoolWithTag(entry, EXCLUDE_ALLOC_TAG);
		LogWarning("Warning, exclude file list is not NULL : %p", cntx);
		return STATUS_ACCESS_DENIED;
	}

	// Push new list entry to context

	if (Type == ExcludeRegKey || Type == ExcludeRegValue)
	{
		// We should add new entry in alphabet order
		head = (PEXCLUDE_FILE_LIST_ENTRY)cntx->listHead.Flink;
		while (head != (PEXCLUDE_FILE_LIST_ENTRY)&cntx->listHead)
		{
			INT res = RtlCompareUnicodeString(&entry->path.fullPath, &head->path.fullPath, TRUE);
			if (res <= 0)
				break;

			head = (PEXCLUDE_FILE_LIST_ENTRY)head->list.Flink;
		}
	}
	else
	{
		head = (PEXCLUDE_FILE_LIST_ENTRY)&cntx->listHead;
	}

	// Parent GUID is used when we want to link few entries in a group with one master,
	// in this case parent GUID should be any valid entry GUID. When we remove parent entry
	// all it's children will be removed too
	entry->parentGuid = ParentId;

	ExAcquireFastMutex(&cntx->listLock);

	if (entry->parentGuid)
		cntx->childCounter++;

	entry->guid = cntx->guidCounter++;
	InsertTailList((PLIST_ENTRY)head, (PLIST_ENTRY)entry);

	ExReleaseFastMutex(&cntx->listLock);

	*EntryId = entry->guid;

	return STATUS_SUCCESS;
}

BOOLEAN FillDirectoryFromPath(PEXCULE_FILE_PATH path, PUNICODE_STRING filePath)
{
	USHORT i, count;
	LPWSTR buffer = filePath->Buffer;

	count = filePath->Length / sizeof(WCHAR);
	if (count < 1)
		return FALSE;

	i = count;
	do
	{
		i--;

		if (buffer[i] == L'\\')
		{
			if (i + 1 >= count)
				return FALSE;

			path->fileName.Buffer = buffer + i + 1;
			path->fileName.Length = (count - i - 1) * sizeof(WCHAR);
			path->fileName.MaximumLength = path->fileName.Length;

			path->fullPath = *filePath;

			path->dirName.Buffer = filePath->Buffer;
			path->dirName.Length = i * sizeof(WCHAR);
			path->dirName.MaximumLength = path->dirName.Length;

			return TRUE;
		}
	} while (i > 0);

	return FALSE;
}

NTSTATUS RemoveAllExcludeListEntries(ExcludeContext Context)
{
	PEXCLUDE_FILE_CONTEXT cntx = (PEXCLUDE_FILE_CONTEXT)Context;
	PEXCLUDE_FILE_LIST_ENTRY entry;

	ExAcquireFastMutex(&cntx->listLock);

	entry = (PEXCLUDE_FILE_LIST_ENTRY)cntx->listHead.Flink;
	while (entry != (PEXCLUDE_FILE_LIST_ENTRY)&cntx->listHead)
	{
		PEXCLUDE_FILE_LIST_ENTRY remove = entry;
		entry = (PEXCLUDE_FILE_LIST_ENTRY)entry->list.Flink;
		if (remove->parentGuid)
		{
			ASSERT(cntx->childCounter > 0);
			cntx->childCounter--;
		}
		RemoveEntryList((PLIST_ENTRY)remove);
		ExFreePoolWithTag(remove, EXCLUDE_ALLOC_TAG);
	}

	ASSERT(cntx->childCounter == 0);

	ExReleaseFastMutex(&cntx->listLock);

	return STATUS_SUCCESS;
}

NTSTATUS RemoveExcludeListEntry(ExcludeContext Context, ExcludeEntryId EntryId)
{
	NTSTATUS status = STATUS_NOT_FOUND;
	PEXCLUDE_FILE_CONTEXT cntx = (PEXCLUDE_FILE_CONTEXT)Context;
	PEXCLUDE_FILE_LIST_ENTRY entry;

	ExAcquireFastMutex(&cntx->listLock);

	entry = (PEXCLUDE_FILE_LIST_ENTRY)cntx->listHead.Flink;
	while (entry != (PEXCLUDE_FILE_LIST_ENTRY)&cntx->listHead)
	{
		if (EntryId == entry->guid)
		{
			RemoveEntryList((PLIST_ENTRY)entry);
			ExFreePoolWithTag(entry, EXCLUDE_ALLOC_TAG);
			status = STATUS_SUCCESS;
			break;
		}

		entry = (PEXCLUDE_FILE_LIST_ENTRY)entry->list.Flink;
	}

	if (cntx->childCounter)
	{
		entry = (PEXCLUDE_FILE_LIST_ENTRY)cntx->listHead.Flink;
		while (entry != (PEXCLUDE_FILE_LIST_ENTRY)&cntx->listHead)
		{
			PEXCLUDE_FILE_LIST_ENTRY remove = entry;
			entry = (PEXCLUDE_FILE_LIST_ENTRY)entry->list.Flink;

			if (EntryId == remove->parentGuid)
			{
				ASSERT(cntx->childCounter > 0);
				cntx->childCounter--;
				RemoveEntryList((PLIST_ENTRY)remove);
				ExFreePoolWithTag(remove, EXCLUDE_ALLOC_TAG);
			}

		}
	}

	ExReleaseFastMutex(&cntx->listLock);

	return status;
}
