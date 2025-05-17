#pragma once

#ifndef NT_SUCCESS
   #define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif
#define FILE_PIPE_BYTE_STREAM_TYPE          0x00000000
#define FILE_PIPE_MESSAGE_TYPE              0x00000001
#define FILE_PIPE_BYTE_STREAM_MODE          0x00000000
#define FILE_PIPE_MESSAGE_MODE              0x00000001
#define FILE_PIPE_QUEUE_OPERATION           0x00000000
#define FILE_PIPE_COMPLETE_OPERATION        0x00000001
#define OBJ_CASE_INSENSITIVE                0x00000040
#define FSCTL_PIPE_LISTEN                   0x110008

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
    ULONG MaximumMessageSize;
    ULONG MailslotQuota;
    ULONG NextMessageSize;
    ULONG MessagesAvailable;
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

HANDLE create_named_pipe(LPCSTR lpName);
HANDLE create_named_pipe_file(LPCSTR lpName);
BOOL connect_named_pipe(HANDLE hNamedPipe);
BOOL write_file(HANDLE hFile, PVOID pBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten);
BOOL read_file(HANDLE hFile, PVOID pBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead);
HANDLE create_file_mapping(DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow);
LPVOID map_view_of_file(HANDLE hFile);
HANDLE create_mailslot(LPCSTR lpName);
HANDLE create_mailslot_file(LPCSTR lpName);
BOOL get_mailslot_info(HANDLE hMailslot, LPDWORD lpNextSize, LPDWORD lpMessageCount);
