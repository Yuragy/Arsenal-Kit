# Post Exploitation Loader Example

This loader example serves as a demonstration of User Defined Reflective Loader for Post Exploitation DLLs.

## Quick Start Guide

To get started, use the instructions provided below.

### Debug Build

Exporting a raw Post Exploitation DLL is not a practical option. For debugging purposes, it is highly advisable to utilize a 'dummy' DLL.
As an illustration, the following code provides a straightforward DLL implementation that displays all information passed to the loader.

```c++
#include <Windows.h>
#include <cstdio>

typedef struct {
    char* start;
    DWORD  length;
    DWORD  offset;
} RDATA_SECTION;


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    RDATA_SECTION* rdata = (RDATA_SECTION*)lpReserved;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        printf("DLL_PROCESS_ATTACH:\n");
        printf("\tDLL Base Address: %p\n", hModule);
        printf("\trdata.start  %p\n", rdata->start);
        printf("\trdata.length %i\n", rdata->length);
        printf("\trdata.offset %i\n", rdata->offset);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    case 4:
        printf("Reason 4:\n");
        printf("\tLoader Base Address: %p\n", hModule);
        printf("\tLoader Argument:     %p\n", lpReserved);
        break;
    }
    return TRUE;
}
```

To start Debugging:
* Compile our Debug DLL.
* Add the DLL to the project - `py.exe .\udrl.py xxd <path\to\dummy.dll> .\library\DebugDLL.x64.h`.
* Start the Visual Studio Debugger (make sure to right-click postex-loader in the
Solution Explorer->Set As Startup Project).

### Release Build

To use the Release build:
* Compile the `Release` build.
* Load (`bin/postex-loader/prepend-postex-udrl.cna`) to Cobalt Strike.