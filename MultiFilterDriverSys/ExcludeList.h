#pragma once

//#include <ntifs.h>
#include <Ntddk.h>

enum ExcludeObjectType {
	ExcludeFile,
	ExcludeDirectory,
	ExcludeRegKey,
	ExcludeRegValue,
	ExcludeMaxType,
};

typedef PVOID ExcludeContext;
typedef ExcludeContext* PExcludeContext;

typedef ULONGLONG ExcludeEntryId;
typedef ExcludeEntryId* PExcludeEntryId;

typedef ULONGLONG ExcludeEnumId;
typedef ExcludeEnumId* PExcludeEnumId;

NTSTATUS InitializeExcludeListContext(PExcludeContext Context, UINT32 Type);
VOID DestroyExcludeListContext(ExcludeContext Context);

NTSTATUS AddExcludeListFile(ExcludeContext Context, PUNICODE_STRING FilePath, PExcludeEntryId EntryId, ExcludeEntryId ParentId);
NTSTATUS AddExcludeListDirectory(ExcludeContext Context, PUNICODE_STRING DirPath, PExcludeEntryId EntryId, ExcludeEntryId ParentId);
NTSTATUS AddExcludeListEntry(ExcludeContext Context, PUNICODE_STRING FilePath, UINT32 Type, PExcludeEntryId EntryId, ExcludeEntryId ParentId);

NTSTATUS RemoveExcludeListEntry(ExcludeContext Context, ExcludeEntryId EntryId);
NTSTATUS RemoveAllExcludeListEntries(ExcludeContext Context);

BOOLEAN CheckExcludeListDirectory(ExcludeContext Context, PCUNICODE_STRING Path);
BOOLEAN CheckExcludeListDirFile(ExcludeContext Context, PCUNICODE_STRING Dir, PCUNICODE_STRING File);
