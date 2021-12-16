#include <windows.h>
#include "Api.h"
#include "GetGDIObjects.h"

template <class TPtr>
struct GDICELL
{
    TPtr pKernelAddress;
    USHORT wProcessId;
    USHORT wCount;
    USHORT wUpper;
    USHORT wType;
    TPtr pUserAddress;
};

typedef NTSTATUS (NTAPI* NtQueryInformationProcessPtr)(
    IN HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL);

typedef NTSTATUS (NTAPI* NtWow64ReadVirtualMemory64Ptr)(
    IN HANDLE ProcessHandle,
    IN PVOID64 BaseAddress,
    OUT PVOID Buffer,
    IN ULONG64 Size,
    OUT PULONG64 NumberOfBytesRead);

template <typename TPtr>
struct PROCESS_BASIC_INFORMATION
{
    TPtr Reserved1;
    TPtr PebBaseAddress;
    TPtr Reserved2[2];
    TPtr UniqueProcessId;
    TPtr Reserved3;
};

auto const NtQueryInformationProcess = Api::MakeNTApiCallSite(
    (NtQueryInformationProcessPtr)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"),
    Api::CheckNTApiFunctionResult);

auto const NtWow64QueryInformationProcess64 = Api::MakeNTApiCallSite(
    (NtQueryInformationProcessPtr)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtWow64QueryInformationProcess64"),
    Api::CheckNTApiFunctionResult);

auto const NtWow64ReadVirtualMemory64 = Api::MakeNTApiCallSite(
    (NtWow64ReadVirtualMemory64Ptr)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtWow64ReadVirtualMemory64"),
    Api::CheckNTApiFunctionResult);

std::vector<HGDIOBJ> GetGDIObjects()
{
    std::vector<HGDIOBJ> gdiObjects;

    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);

    // Unfortunately, the offset are not documented
    auto const GdiSharedHandleTableOffset = PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture ? 0xf8u : 0x94u;

    auto const tableCount = 65536u;

    auto const ProcessBasicInformation = 0;

    auto const collectGdiObjects = [&](auto const& gdiTable)
    {
        for (auto i = 0u; i < tableCount; i++)
        {
            auto const cell = gdiTable[i];

            if (cell.wProcessId != GetCurrentProcessId())
                // It happens
                continue;

            auto const gdiHandle = (HGDIOBJ)((cell.wUpper << 16) + i);

            gdiObjects.push_back(gdiHandle);
        }
    };

    BOOL isWow64Process;
    IsWow64Process(GetCurrentProcess(), &isWow64Process);

    if (isWow64Process)
    {
        PROCESS_BASIC_INFORMATION<PVOID64> processBasicInformation;
        DWORD returnLength = 0;
        NtWow64QueryInformationProcess64(
            GetCurrentProcess(),
            ProcessBasicInformation,
            &processBasicInformation,
            sizeof(processBasicInformation),
            &returnLength);

        std::vector<BYTE> peb(GdiSharedHandleTableOffset + 8);

        auto const processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

        NtWow64ReadVirtualMemory64(
            processHandle,
            processBasicInformation.PebBaseAddress,
            &peb[0],
            (DWORD)peb.size(),
            nullptr);

        CloseHandle(processHandle);

        auto const gdiTable = (GDICELL<PVOID64>*)*(LPVOID*)(&peb[0] + GdiSharedHandleTableOffset);

        collectGdiObjects(gdiTable);
    }
    else
    {
        PROCESS_BASIC_INFORMATION<PVOID> processBasicInformation;
        DWORD returnLength = 0;
        NtQueryInformationProcess(
            GetCurrentProcess(),
            ProcessBasicInformation,
            &processBasicInformation,
            sizeof(processBasicInformation),
            &returnLength);

        auto const gdiTable = (GDICELL<PVOID>*)*(LPVOID*)((PBYTE)processBasicInformation.PebBaseAddress + GdiSharedHandleTableOffset);

        collectGdiObjects(gdiTable);
    }

    return gdiObjects;
}
