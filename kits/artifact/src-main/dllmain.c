#include "windows.h"
#if USE_SYSCALLS == 1
#include "syscalls.h"
#endif

void start(HINSTANCE handle);

HINSTANCE myhandle;

DWORD ThreadProc(LPVOID param) {
	start(myhandle);
	return 0;
}

BOOL WINAPI DllMain (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			/* create a thread here... if a start function blocks--it could hold up other
			   DLLs loading and then nothing will appear to work */
			myhandle = hDll;
#if USE_SYSCALLS == 1
         HANDLE thandle;
         NtCreateThreadEx(&thandle, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), &ThreadProc, NULL, 0, 0, 0, 0, NULL);
#else
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadProc, (LPVOID)NULL, 0, NULL);
#endif
		break;
	}
	return TRUE;
}

STDAPI DllRegisterServer(void) {
	return (HRESULT)S_OK;
}

STDAPI DllUnregisterServer(void) {
	return (HRESULT)S_OK;
}

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID *ppv ) {
	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllRegisterServerEx( LPCTSTR lpszModuleName ) {
	return (HRESULT)S_OK;
}

/* rundll32.exe entry point Start */
void CALLBACK StartW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
	while (TRUE)
		WaitForSingleObject(GetCurrentProcess(), 60000);
}
