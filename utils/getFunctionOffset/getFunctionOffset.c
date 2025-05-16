#include <windows.h>
#include <stdio.h>
#include <winnt.h>
#include <stdlib.h>

VOID GetFunctionOffset(VOID *lib, LPCSTR dllName, LPCSTR funcName, DWORD offsetInFunc) {
    void * uiFuncAddress = GetProcAddress(lib, funcName);
    if (!uiFuncAddress) {
        printf("Failed to find function %s\n", funcName);
        return;
    }

    DWORD offset = (DWORD)((char *)uiFuncAddress - (char*) lib);

    printf("function:         %s\n", funcName);
    printf("dllBaseAddr:      %p\n", lib);
    printf("functionAddr:     %p\n", uiFuncAddress);
    printf("offset:           0x%lx\n", offset);
    printf("returnAddrOffset: 0x%lx\n", offset + offsetInFunc);
    printf("generated code:   set_frame_info(&callstack[i++], L\"%s\", 0, 0x%lx, 0, FALSE);\n", dllName, offset + offsetInFunc);
}

int main(int argc, char **argv) {

    if (argc < 4) {
       printf("Error not enough arguments\n\n");
       printf("Usage: %s <dllName> <functionName> <offsetInFunction>\n", argv[0]);
       ExitProcess(0);
    }

    DWORD offsetInFunc = 0;
    CHAR *dllName  = argv[1];
    CHAR *funcName = argv[2];

    char *endPtr = NULL;
    offsetInFunc = (DWORD)strtol(argv[3], &endPtr, 16);
    if ((offsetInFunc == 0 && errno != 0) || (endPtr != NULL && *endPtr != '\0')) {
       printf("Error offsetInFunction '%s' is not valid.\n", argv[3]);
       ExitProcess(0);
    }


    HANDLE hDll = LoadLibrary(dllName);
    if(hDll == NULL) {
        printf("Failed to load module %s\n", dllName);
        ExitProcess(0);
    }

    GetFunctionOffset(hDll, dllName, funcName, offsetInFunc);

    CloseHandle(hDll);

    return 0;
}
