#pragma once

#include <ntddk.h>

typedef struct
{
    BOOLEAN _initialized;
    PMDL _mdl;
    PUINT8 data;
    SIZE_T length;
} UserBuffer;

NTSTATUS
initializeUserBuffer
(
    PVOID userPtr,
    SIZE_T bufferLength,
    UserBuffer* userBufferPtr
);

void
finalizeUserBuffer
(
    UserBuffer* userBufferPtr
);