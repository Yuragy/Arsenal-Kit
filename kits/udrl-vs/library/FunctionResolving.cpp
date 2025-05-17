#include "FunctionResolving.h"
#include "LoaderTypes.h"
#include "StdLib.h"

/*******************************************************************
 * To avoid problems with function positioning, do not add any new
 * functions above this pragma directive.
********************************************************************/
#pragma code_seg(".text$d")

/**
 * Find the address of a target function within a loaded module
 *
 * @param pebAddress A pointer to the Process Environment Block (PEB)
 * @param moduleHash A hash of the target module name
 * @param functionHash A hash of the target function name
 * @return A pointer to the target function
*/
ULONG_PTR GetProcAddressByHash(_PPEB pebAddress, DWORD moduleHash, DWORD functionHash) {
    // Get the processes loaded modules. ref: http://msdn.microsoft.com/en-us/library/aa813708(VS.85).aspx
    PPEB_LDR_DATA ldrData = (PPEB_LDR_DATA)(pebAddress)->pLdr;

    // Get the first entry of the InMemoryOrder module list
    PLDR_DATA_TABLE_ENTRY currentLdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)ldrData->InMemoryOrderModuleList.Flink;
    while (currentLdrDataTableEntry) {
        // Get pointer to current module's name (unicode string)
        PWSTR dllName = currentLdrDataTableEntry->BaseDllName.pBuffer;

        // Set counter to the length for the loop
        USHORT nameLength = currentLdrDataTableEntry->BaseDllName.Length / 2;

        // Compute the hash of the module name...
        DWORD moduleNameHash = 0;
        do {
            moduleNameHash = _rotr(moduleNameHash, HASH_KEY);
            // Normalize to uppercase if the module name is in lowercase
            if (*dllName >= 'a') {
                moduleNameHash += *dllName - 0x20;
            }
            else {
                moduleNameHash += *dllName;
            }
            dllName++;
        } while (--nameLength);

        if (moduleNameHash == moduleHash) {
            // Get this module's base address
            ULONG_PTR moduleBaseAddress = (ULONG_PTR)currentLdrDataTableEntry->DllBase;

            // Get the VA of the module's PE Header
            PIMAGE_DOS_HEADER moduleDosHeader = (PIMAGE_DOS_HEADER)moduleBaseAddress;
            PIMAGE_NT_HEADERS modulePEHeader = (PIMAGE_NT_HEADERS)(moduleBaseAddress + moduleDosHeader->e_lfanew);

            // ExportDataDirectoryEntry = the address of the module's export directory entry
            PIMAGE_DATA_DIRECTORY exportDataDirectoryEntry = &modulePEHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

            // Get the VA of the export directory
            PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)(moduleBaseAddress + exportDataDirectoryEntry->VirtualAddress);

            // Get the VA for the array of name pointers
            ULONG_PTR nameArray = moduleBaseAddress + exportDirectory->AddressOfNames;

            // Get the VA for the array of name ordinals
            ULONG_PTR ordinalArray = moduleBaseAddress + exportDirectory->AddressOfNameOrdinals;

            // Loop while we still have imports to find
            while (nameArray) {
                // Compute the hash values for this function name
                 DWORD functionNameHash = RunTimeHash((char*)(moduleBaseAddress + DEREF_32(nameArray)));

                // If we have found a function we want, get its virtual address
                if (functionNameHash == functionHash) {
                    // Get the VA for the array of addresses
                    ULONG_PTR addressArray = moduleBaseAddress + exportDirectory->AddressOfFunctions;

                    // Use this function's name ordinal as an index into the array of name pointers
                    addressArray += DEREF_16(ordinalArray) * sizeof(DWORD);

                    // Store this function's VA
                    return moduleBaseAddress + DEREF_32(addressArray);
                }
                // Get the next exported function name
                nameArray += sizeof(DWORD);

                // Get the next exported function name ordinal
                ordinalArray += sizeof(WORD);
            }
        }
        // Get the next entry
        currentLdrDataTableEntry = *(PLDR_DATA_TABLE_ENTRY*)currentLdrDataTableEntry;
    }
    return NULL;
}

/**
 * Resolve the base loader functions
 *
 * @param pebAddress A pointer to the Process Environment Block (PEB)
 * @param winApi A pointer to a structure of WINAPI pointers
 * @return A Boolean value to indicate success
*/
BOOL ResolveBaseLoaderFunctions(_PPEB pebAddress, PWINDOWSAPIS winApi) {
    winApi->LoadLibraryA = (LOADLIBRARYA)GetProcAddressByHash(pebAddress, KERNEL32DLL_HASH, LOADLIBRARYA_HASH);
    if (winApi->LoadLibraryA == NULL) {
        PRINT("[-] Failed to find address of key loader function. Exiting..\n");
        return FALSE;
    }
    winApi->GetProcAddress = (GETPROCADDRESS)GetProcAddressByHash(pebAddress, KERNEL32DLL_HASH, GETPROCADDRESS_HASH);
    if (winApi->GetProcAddress == NULL) {
        PRINT("[-] Failed to find address of key loader function. Exiting..\n");
        return FALSE;
    }
    winApi->VirtualAlloc = (VIRTUALALLOC)GetProcAddressByHash(pebAddress, KERNEL32DLL_HASH, VIRTUALALLOC_HASH);
    if (winApi->VirtualAlloc == NULL) {
        PRINT("[-] Failed to find address of key loader function. Exiting..\n");
        return FALSE;
    }
    winApi->NtFlushInstructionCache = (NTFLUSHINSTRUCTIONCACHE)GetProcAddressByHash(pebAddress, NTDLLDLL_HASH, NTFLUSHINSTRUCTIONCACHE_HASH);
    if (winApi->NtFlushInstructionCache == NULL) {
        PRINT("[-] Failed to find address of key loader function. Exiting..\n");
        return FALSE;
    }
    return TRUE;
}
