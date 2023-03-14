# Sample using ntdll to list all Device Objects

```c
#include <Windows.h>
#include <tchar.h>
#include <cstdio>
#include <winternl.h>

NTSTATUS (NTAPI* NtOpenDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
NTSTATUS (NTAPI* NtQueryDirectoryObject)(HANDLE, PVOID, ULONG, BOOLEAN, BOOLEAN, PULONG, PULONG);
VOID (NTAPI* RtlInitUnicodeString_)(PUNICODE_STRING, PCWSTR);
NTSTATUS (NTAPI* NtClose_)(HANDLE);

#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L) // ntsubauth
#endif // STATUS_SUCCESS
#ifndef STATUS_MORE_ENTRIES
#define STATUS_MORE_ENTRIES              ((NTSTATUS)0x00000105L)
#endif // STATUS_MORE_ENTRIES
#ifndef STATUS_NO_MORE_ENTRIES
#define STATUS_NO_MORE_ENTRIES           ((NTSTATUS)0x8000001AL)
#endif // STATUS_NO_MORE_ENTRIES

int PrintDevices()
{
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING objname;
    HANDLE hDeviceDir = NULL;
    RtlInitUnicodeString_(&objname, L"\\Device");
    InitializeObjectAttributes(&oa, &objname, 0, NULL, NULL);
    ntStatus = NtOpenDirectoryObject(&hDeviceDir, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &oa);
    if(NT_SUCCESS(ntStatus))
    {
        size_t const bufSize = 0x10000;
        BYTE buf[bufSize] = {0};
        ULONG start = 0, idx = 0, bytes;
        BOOLEAN restart = TRUE;
        for(;;)
        {
            ntStatus = NtQueryDirectoryObject(hDeviceDir, PBYTE(buf), bufSize, FALSE, restart, &idx, &bytes);
            if(NT_SUCCESS(ntStatus))
            {
                POBJECT_DIRECTORY_INFORMATION const pdilist = reinterpret_cast<POBJECT_DIRECTORY_INFORMATION>(PBYTE(buf));
                for(ULONG i = 0; i < idx - start; i++)
                {
                    if(0 == wcsncmp(pdilist[i].TypeName.Buffer, L"Device", pdilist[i].TypeName.Length / sizeof(WCHAR)))
                    {
                        _tprintf(_T("%s\n"), pdilist[i].Name.Buffer);
                    }
                }
            }
            if(STATUS_MORE_ENTRIES == ntStatus)
            {
                start = idx;
                restart = FALSE;
                continue;
            }
            if((STATUS_SUCCESS == ntStatus) || (STATUS_NO_MORE_ENTRIES == ntStatus))
            {
                break;
            }
        }
        (void)NtClose_(hDeviceDir);
        return 0;
    }
    _tprintf(_T("Failed NtOpenDirectoryObject with 0x%08X"), ntStatus);
    return 1;
}

int _tmain(int /*argc*/, _TCHAR** /*argv*/)
{
    HMODULE hNtDll = ::GetModuleHandle(_T("ntdll.dll"));
    *(FARPROC*)&NtOpenDirectoryObject = ::GetProcAddress(hNtDll, "NtOpenDirectoryObject");
    *(FARPROC*)&NtQueryDirectoryObject = ::GetProcAddress(hNtDll, "NtQueryDirectoryObject");
    *(FARPROC*)&RtlInitUnicodeString_ = ::GetProcAddress(hNtDll, "RtlInitUnicodeString");
    *(FARPROC*)&NtClose_ = ::GetProcAddress(hNtDll, "NtClose");
    if (!NtOpenDirectoryObject || !NtQueryDirectoryObject || !RtlInitUnicodeString_ || !NtClose_)
    {
        _tprintf(_T("Failed to retrieve ntdll.dll function pointers\n"));
        return 1;
    }
    return PrintDevices();
}
```
