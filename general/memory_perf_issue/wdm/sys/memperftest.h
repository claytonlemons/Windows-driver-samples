/*
    Defines the IOCTL codes and other constants that will be used by this driver. 
*/

#pragma once

#include <basetsd.h>
#include "types.h"

#define MEMPERFTEST_DEVICETYPE 40000

#define IOCTL_MEMPERFTEST_ALLOCATEMEMORY \
    CTL_CODE(MEMPERFTEST_DEVICETYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MEMPERFTEST_FREEMEMORY \
    CTL_CODE(MEMPERFTEST_DEVICETYPE, 0x901, METHOD_BUFFERED, FILE_READ_DATA)

#define DRIVER_NAME "MEMPERFTEST"

#pragma push pack 1
typedef struct
{
    SIZE_T nonPagedMemorySizeInBytes;
    SIZE_T contiguousMemorySizeInBytes;
    SIZE_T numContiguousMemoryChunks;
    VIRTUAL_ADDRESS nonPagedMemoryAddress;
    PVOID contiguousMemoryAddressesUserPtr;
    UINT64 elapsedTimeInMicroseconds;
} AllocateMemoryParams;

typedef struct
{
    VIRTUAL_ADDRESS nonPagedMemoryAddress;
    SIZE_T numContiguousMemoryChunks;
    PVOID contiguousMemoryAddressesUserPtr;
} FreeMemoryParams;
