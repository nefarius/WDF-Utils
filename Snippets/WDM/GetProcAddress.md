# GetProcAddress implementation - for the Kernel

For dynamically calling functions, [`MmGetSystemRoutineAddress`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-mmgetsystemroutineaddress) will only work for exports from `NtosKrnl.exe`, but what if you wish to get a pointer to a function of an export driver (a.k.a. kernel DLL)? The following snippet has been tested on Windows 10 version 1507 and upwards.

## Header

```c
#pragma once

// Structure representing a loaded module
typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY
{
    PVOID Unknown1;
    PVOID Unknown2;
    PVOID Base;
    ULONG Size;
    ULONG Flags;
    USHORT Index;
    USHORT NameLength;
    USHORT LoadCount;
    USHORT PathLength;
    CHAR ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;

// Structure representing the loaded module information
typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG Count;
    SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

// Function prototype for ZwQuerySystemInformation
NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY64 InLoadOrderLinks;
    PVOID ExceptionTable;
    ULONG ExceptionTableSize;
    PVOID GpValue;
    PVOID NonPagedDebugInfo;
    PVOID ImageBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullImageName;
    UNICODE_STRING BaseImageName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY64 HashLinks;
    PVOID SectionPointer;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    PVOID LoadedImports;
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef PVOID (NTAPI* t_RtlImageDirectoryEntryToData)(
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
);


_Success_(return == STATUS_SUCCESS)
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FindDriverBaseAddress(
    _In_ STRING ModuleName,
    _Inout_ PVOID* ModuleBase
);

_Success_(return == STATUS_SUCCESS)
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FundExportedFunctionAddress(
    _In_ PVOID ModuleBase,
    _In_ STRING FunctionName,
    _Inout_ PVOID* FunctionAddress
);
```

## Source

```c
#include <ntifs.h>
#include <ntimage.h>


// 
// Finds the base address of a driver module
// 
_Success_(return == STATUS_SUCCESS)
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FindDriverBaseAddress(
    _In_ STRING ModuleName,
    _Inout_ PVOID* ModuleBase
)
{
    ULONG bufferSize = 0;
    PSYSTEM_MODULE_INFORMATION moduleInfo = NULL;

    const ULONG SystemModuleInformation = 11;

    // Query the required buffer size for module information
    NTSTATUS status = ZwQuerySystemInformation(
        SystemModuleInformation,
        &bufferSize,
        0,
        &bufferSize
    );

    if (status != STATUS_INFO_LENGTH_MISMATCH)
    {
        return status;
    }

#pragma warning(disable:4996)
    // Allocate memory for the module information
    moduleInfo = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(
        NonPagedPool,
        bufferSize,
        BTHPS_POOL_TAG
    );
#pragma warning(default:4996)

    if (moduleInfo == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Retrieve the module information
    status = ZwQuerySystemInformation(
        SystemModuleInformation,
        moduleInfo,
        bufferSize,
        NULL
    );

    if (!NT_SUCCESS(status))
    {
        ExFreePool(moduleInfo);
        return status;
    }

    STRING currentImageName;

    status = STATUS_NOT_FOUND;
    // Iterate through the loaded modules and find the desired module
    for (ULONG i = 0; i < moduleInfo->Count; i++)
    {
        RtlInitAnsiString(&currentImageName, moduleInfo->Module[i].ImageName);

        if (0 == RtlCompareString(&ModuleName, &currentImageName, TRUE))
        {
            // Found the module, store the base address
            if (ModuleBase)
            {
                status = STATUS_SUCCESS;
                *ModuleBase = moduleInfo->Module[i].Base;
            }
            break;
        }
    }

    ExFreePool(moduleInfo);

    return status;
}

_Success_(return == STATUS_SUCCESS)
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FundExportedFunctionAddress(
    _In_ PVOID ModuleBase,
    _In_ STRING FunctionName,
    _Inout_ PVOID* FunctionAddress
)
{
    NTSTATUS status = STATUS_NOT_FOUND;
    ULONG exportSize;

    DECLARE_CONST_UNICODE_STRING(routineName, L"RtlImageDirectoryEntryToData");

    const t_RtlImageDirectoryEntryToData fp_RtlImageDirectoryEntryToData =
        (t_RtlImageDirectoryEntryToData)MmGetSystemRoutineAddress((PUNICODE_STRING)&routineName);

    if (fp_RtlImageDirectoryEntryToData == NULL)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    // Retrieve the export directory information
    const PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)fp_RtlImageDirectoryEntryToData(
        ModuleBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &exportSize
    );

    if (exportDirectory == NULL)
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    STRING currentFunctionName;
    
    const PULONG functionAddresses = (PULONG)((ULONG_PTR)ModuleBase + exportDirectory->AddressOfFunctions);
    const PULONG functionNames = (PULONG)((ULONG_PTR)ModuleBase + exportDirectory->AddressOfNames);
    const PUSHORT functionOrdinals = (PUSHORT)((ULONG_PTR)ModuleBase + exportDirectory->AddressOfNameOrdinals);

    for (DWORD i = 0; i < exportDirectory->NumberOfNames; i++)
    {
        const char* functionName = (const char*)((ULONG_PTR)ModuleBase + functionNames[i]);
        const USHORT functionOrdinal = functionOrdinals[i];
        UNREFERENCED_PARAMETER(functionOrdinal);

        const ULONG functionRva = functionAddresses[i];
        const PVOID functionAddress = (PVOID)((ULONG_PTR)ModuleBase + functionRva);

        RtlInitAnsiString(&currentFunctionName, functionName);

        if (0 == RtlCompareString(&FunctionName, &currentFunctionName, TRUE))
        {
            if (FunctionAddress)
            {
                status = STATUS_SUCCESS;
                *FunctionAddress = functionAddress;
            }
            break;
        }
    }

    return status;
}
```

## Usage example

```c
// full path to module of interest (CAUTION: must be loaded!)
const STRING targetModuleName = RTL_CONSTANT_STRING("\\SystemRoot\\System32\\Drivers\\WppRecorder.sys");
// exported function name
const STRING functionName = RTL_CONSTANT_STRING("imp_WppRecorderReplay");

PVOID driverBaseAddress = NULL, functionAddress = NULL;

if (NT_SUCCESS(FindDriverBaseAddress(targetModuleName, &driverBaseAddress)))
{
    if (NT_SUCCESS(FundExportedFunctionAddress(driverBaseAddress, functionName, &functionAddress)))
    {
        // Found imp_WppRecorderReplay, you can now safely call "functionAddress"
    }
    else
    {
        // imp_WppRecorderReplay not available on this Windows version
    }
}
else
{
    // Failed to find module base address
}
```
