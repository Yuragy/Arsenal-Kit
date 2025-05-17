#include <windows.h>
#include <stdio.h>
#include "syscalls.h"
#include "utils.h"

void initialize_object_attributes_with_name(POBJECT_ATTRIBUTES ObjectAttributes, LPCSTR lpName, PUNICODE_STRING ucName) {
   wchar_t wcName[MAX_PATH] = { 0 };

   wcsncpy(ucName->Buffer, L"\\??\\", MAX_PATH);
   mbstowcs(wcName, &lpName[4], MAX_PATH);
   wcsncat(ucName->Buffer, wcName, MAX_PATH);
   ucName->Length = (USHORT)wcsnlen(ucName->Buffer, MAX_PATH);
   ucName->Length *= 2;
   ucName->MaximumLength = ucName->Length + 2;
   InitializeObjectAttributes(ObjectAttributes,
                              ucName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);
}

void initialize_object_attributes_with_sqs(POBJECT_ATTRIBUTES ObjectAttributes, PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService) {
   SecurityQualityOfService->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
   SecurityQualityOfService->ImpersonationLevel = DEFAULT_IMPERSONATION_LEVEL;
   SecurityQualityOfService->ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
   SecurityQualityOfService->EffectiveOnly = TRUE;
   ObjectAttributes->SecurityQualityOfService = SecurityQualityOfService;
}

HANDLE create_named_pipe(LPCSTR lpName) {
   HANDLE hNamedPipe = NULL;
   NTSTATUS Status;
   ACCESS_MASK DesiredAccess;
   OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
   SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService = { 0 };
   IO_STATUS_BLOCK IoStatusBlock = { 0 };
   ULONG CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
   ULONG WriteModeMessage = FILE_PIPE_BYTE_STREAM_TYPE;
   ULONG ReadModeMessage = FILE_PIPE_BYTE_STREAM_MODE;
   ULONG NonBlocking = FILE_PIPE_QUEUE_OPERATION;
   LARGE_INTEGER DefaultTimeOut;
   UNICODE_STRING ucName = { 0 };
   wchar_t ucBuffer[MAX_PATH] = { 0 };

   DefaultTimeOut.QuadPart = -500000; /* Use default timeout of 50 ms */
   ucName.Buffer = ucBuffer;

   /* Setup the default Desired Access */
   DesiredAccess = SYNCHRONIZE | GENERIC_WRITE | (PIPE_ACCESS_OUTBOUND & (WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY));

   /* Initialize the object attributes */
   initialize_object_attributes_with_name(&ObjectAttributes, lpName, &ucName);
   initialize_object_attributes_with_sqs(&ObjectAttributes, &SecurityQualityOfService);

   Status = NtCreateNamedPipeFile(&hNamedPipe,
                                  DesiredAccess,
                                  &ObjectAttributes,
                                  &IoStatusBlock,
                                  FILE_SHARE_READ,
                                  FILE_OPEN_IF,
                                  CreateOptions,
                                  WriteModeMessage,
                                  ReadModeMessage,
                                  NonBlocking,
                                  1,
                                  0,
                                  0,
                                  &DefaultTimeOut);

  if (NT_SUCCESS(Status)) {
     return hNamedPipe;
  }

  return NULL;
}

HANDLE create_named_pipe_file(LPCSTR lpName) {
   NTSTATUS Status;
   HANDLE hFile = NULL;
   OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
   SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService = { 0 };
   IO_STATUS_BLOCK IoStatusBlock = { 0 };
   UNICODE_STRING ucName = { 0 };
   wchar_t ucBuffer[MAX_PATH] = { 0 };

   ucName.Buffer = ucBuffer;

   /* Now we can initialize the object attributes */
   initialize_object_attributes_with_name(&ObjectAttributes, lpName, &ucName);
   initialize_object_attributes_with_sqs(&ObjectAttributes, &SecurityQualityOfService);

   Status = NtCreateFile(&hFile,
                         FILE_ATTRIBUTE_NORMAL | GENERIC_READ | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         NULL,
                         FILE_ATTRIBUTE_NORMAL,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_OPEN,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
                         NULL,
                         0);
   if (NT_SUCCESS(Status)) {
      return hFile;
   }

   return NULL;
}

BOOL connect_named_pipe(HANDLE hNamedPipe) {
   NTSTATUS Status;
   IO_STATUS_BLOCK IoStatusBlock = { 0 };

   Status = NtFsControlFile(hNamedPipe,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            FSCTL_PIPE_LISTEN,
                            NULL,
                            0,
                            NULL,
                            0);
   /* wait in case operation is pending */
   if (Status == STATUS_PENDING) {
      if (WaitForSingleObject(hNamedPipe, INFINITE) == WAIT_OBJECT_0) {
         Status = IoStatusBlock.Status;
      }
   }
   if (NT_SUCCESS(Status)) {
      return TRUE;
   }

   return FALSE;
}

BOOL write_file(HANDLE hFile, PVOID pBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten) {
   NTSTATUS Status;
   IO_STATUS_BLOCK IoStatusBlock = { 0 };

   *lpNumberOfBytesWritten = 0;
   Status = NtWriteFile(hFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        pBuffer,
                        nNumberOfBytesToWrite,
                        NULL,
                        NULL);
   /* wait in case operation is pending */
   if (Status == STATUS_PENDING) {
      if (WaitForSingleObject(hFile, INFINITE) == WAIT_OBJECT_0) {
         Status = IoStatusBlock.Status;
      }
   }
   if (NT_SUCCESS(Status)) {
      *lpNumberOfBytesWritten = (DWORD) IoStatusBlock.Information;
      return TRUE;
   }

   return FALSE;
}

BOOL read_file(HANDLE hFile, PVOID pBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) {
   NTSTATUS Status;
   IO_STATUS_BLOCK IoStatusBlock = { 0 };

   *lpNumberOfBytesRead = 0;
   Status = NtReadFile(hFile,
   	                 NULL,
   	                 NULL,
   	                 NULL,
   	                 &IoStatusBlock,
   	                 pBuffer,
   	                 nNumberOfBytesToRead,
   	                 NULL,
   	                 NULL);
   /* wait in case operation is pending */
   if (Status == STATUS_PENDING) {
      if (WaitForSingleObject(hFile, INFINITE) == WAIT_OBJECT_0) {
         Status = IoStatusBlock.Status;
      }
   }
   if (NT_SUCCESS(Status)) {
      *lpNumberOfBytesRead = (DWORD) IoStatusBlock.Information;
      return TRUE;
   }

   return FALSE;
}

HANDLE create_file_mapping(DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow) {
   NTSTATUS Status;
   HANDLE sectionHandle;
   DWORD flProtect = PAGE_EXECUTE_READWRITE;
   ACCESS_MASK DesiredAccess;
   ULONG Attributes;
   LARGE_INTEGER sectionSize;

   /* Set default access */
   DesiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE;

   /* Set the attributes */
   Attributes = flProtect & (SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_NOCACHE | SEC_COMMIT | SEC_LARGE_PAGES);
   flProtect ^= Attributes;
   if (!Attributes) Attributes = SEC_COMMIT;

   /* Set the section size information */
   sectionSize.LowPart = dwMaximumSizeLow;
   sectionSize.HighPart = dwMaximumSizeHigh;

   /* Now create the actual section */
   Status = NtCreateSection(&sectionHandle,
                            DesiredAccess,
                            NULL,
                            &sectionSize,
                            flProtect,
                            Attributes,
                            NULL);
   if (NT_SUCCESS(Status)) {
      return sectionHandle;
   }
   return NULL;
}

LPVOID map_view_of_file(HANDLE hFile) {
   NTSTATUS Status;
   LARGE_INTEGER sectionOffset;
   SIZE_T viewSize = 0;
   LPVOID viewBase = NULL;

   /* Convert the offset */
   sectionOffset.LowPart = 0;
   sectionOffset.HighPart = 0;

   /* Map the section */
   Status = NtMapViewOfSection(hFile,
                               GetCurrentProcess(),
                               &viewBase,
                               0,
                               0,
                               &sectionOffset,
                               &viewSize,
                               ViewShare,
                               0,
                               PAGE_EXECUTE_READWRITE);
   if (NT_SUCCESS(Status)) {
      return viewBase;
   }
   return NULL;
}

HANDLE create_mailslot(LPCSTR lpName) {
   HANDLE handle = NULL;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
   SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService = { 0 };
   IO_STATUS_BLOCK IoStatusBlock = { 0 };
   LARGE_INTEGER DefaultTimeOut;
   UNICODE_STRING ucName = { 0 };
   wchar_t ucBuffer[MAX_PATH] = { 0 };

   DefaultTimeOut.QuadPart = 0xFFFFFFFFFFFFFFFFLL;
   ucName.Buffer = ucBuffer;

   initialize_object_attributes_with_name(&ObjectAttributes, lpName, &ucName);
   initialize_object_attributes_with_sqs(&ObjectAttributes, &SecurityQualityOfService);

   Status = NtCreateMailslotFile(&handle,
	                              GENERIC_READ | SYNCHRONIZE | WRITE_DAC,
	                              &ObjectAttributes,
	                              &IoStatusBlock,
	                              FILE_WRITE_THROUGH,
	                              0,
	                              0,
	                              &DefaultTimeOut);
   if (NT_SUCCESS(Status)) {
      return handle;
   }

   return INVALID_HANDLE_VALUE;
}

HANDLE create_mailslot_file(LPCSTR lpName) {
   NTSTATUS Status;
   HANDLE hFile = NULL;
   OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
   SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService = { 0 };
   IO_STATUS_BLOCK IoStatusBlock = { 0 };
   UNICODE_STRING ucName = { 0 };
   wchar_t ucBuffer[MAX_PATH] = { 0 };

   ucName.Buffer = ucBuffer;

   /* Now we can initialize the object attributes */
   initialize_object_attributes_with_name(&ObjectAttributes, lpName, &ucName);
   initialize_object_attributes_with_sqs(&ObjectAttributes, &SecurityQualityOfService);

   Status = NtCreateFile(&hFile,
                         FILE_ATTRIBUTE_NORMAL | GENERIC_WRITE | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         NULL,
                         FILE_ATTRIBUTE_NORMAL,
                         FILE_SHARE_READ,
                         FILE_OPEN,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
                         NULL,
                         0);
   if (NT_SUCCESS(Status)) {
      return hFile;
   }

   return NULL;
}

BOOL get_mailslot_info(HANDLE hMailslot, LPDWORD lpNextSize, LPDWORD lpMessageCount) {
   FILE_MAILSLOT_QUERY_INFORMATION Buffer;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   Status = NtQueryInformationFile(hMailslot,
                                   &IoStatusBlock,
                                   &Buffer,
                                   sizeof(FILE_MAILSLOT_QUERY_INFORMATION),
                                   FileMailslotQueryInformation);

   if (NT_SUCCESS(Status)) {
      if (lpNextSize) *lpNextSize = Buffer.NextMessageSize;
      if (lpMessageCount) *lpMessageCount = Buffer.MessagesAvailable;
      return TRUE;
   }

   return FALSE;
}
