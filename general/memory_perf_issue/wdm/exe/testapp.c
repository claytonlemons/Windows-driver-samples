#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <sys\memperftest.h>
#include <sys\types.h>
#include "install.h"
#include "error.h"
#define DBG 1
#if DBG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif

VOID __cdecl openDeviceHandle
(
    _In_z_ LPCTSTR driverLocationPtr, 
    _Outptr_ HANDLE* devicePtr, 
    _Inout_ ERR* error
)
{
    if (ERROR_IS_FATAL(error)) return;

    *devicePtr = CreateFile
    (
        "\\\\.\\MemPerfTestDevice",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (*devicePtr == INVALID_HANDLE_VALUE)
    {
        if (GET_LAST_ERROR(error) != ERROR_FILE_NOT_FOUND)
        {
            PRINT_ERROR(error, "CreateFile failed : %d\n");
            return;
        }
        else
        {
            CLEAR_ERROR(error);
        }

        //
        // The driver is not started yet so let us the install the driver.
        // First setup full path to driver name.
        //

        if (!ManageDriver(DRIVER_NAME, driverLocationPtr, DRIVER_FUNC_INSTALL))
        {
            printf("Unable to install driver.\n");

            //
            // Error - remove driver.
            //

            ManageDriver
            (
                DRIVER_NAME,
                driverLocationPtr,
                DRIVER_FUNC_REMOVE
            );

            *error = 32202;
            return;
        }

        *devicePtr = CreateFile
        (
            "\\\\.\\MemPerfTestDevice",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (*devicePtr == INVALID_HANDLE_VALUE) 
        {
            GET_LAST_ERROR(error);
            PRINT_ERROR(error, "Error: CreatFile Failed : %d\n");
            return;
        }
    }
}

VOID allocateMemoryInKernel
(
    HANDLE device, 
    AllocateMemoryParams* params,
    ERR* error
)
{    
    BOOL success;
    DWORD bytesReturned;

    if (ERROR_IS_FATAL(error)) return;

    success = DeviceIoControl
    (
        device,
        (DWORD) IOCTL_MEMPERFTEST_ALLOCATEMEMORY,
        params,
        sizeof(AllocateMemoryParams),
        params,
        sizeof(AllocateMemoryParams),
        &bytesReturned,
        NULL
    );

    if (!success)
    {
        GET_LAST_ERROR(error);
        PRINT_ERROR(error, "Error: Something went wrong with calling the ALLOCATECONTIGUOUSMEMORY IOCTL: %d\n");
    }

    if (bytesReturned != sizeof(AllocateMemoryParams))
    {
        *error = 32202;
        printf("Error: Received %d bytes instead of the expected %Id bytes\n", bytesReturned, sizeof(AllocateMemoryParams));
    }
}

VOID freeMemoryInKernel
(
    HANDLE device,
    FreeMemoryParams* params,
    ERR* error
)
{
    BOOL success;
    DWORD bytesReturned = 0;

    if (ERROR_IS_FATAL(error)) return;
    
    success = DeviceIoControl
    (
        device,
        (DWORD) IOCTL_MEMPERFTEST_FREEMEMORY,
        params,
        sizeof(FreeMemoryParams),
        NULL,
        1,
        &bytesReturned,
        NULL
    );

    if (!success)
    {
        GET_LAST_ERROR(error);
        PRINT_ERROR(error, "Error: Something went wrong with calling the FREECONTIGUOUSMEMORY IOCTL: %d\n");
    }

    if (bytesReturned != 0)
    {
        *error = 32202;
        printf("Error: Received %d bytes instead of the expected %Id bytes\n", bytesReturned, sizeof(FreeMemoryParams));
    }
}

UINT64 __cdecl runTest
(
    HANDLE device,
    SIZE_T nonPagedMemorySizeInBytes,
    SIZE_T contiguousMemorySizeInBytes,
    SIZE_T numContiguousMemoryChunks,
    ERR* error
)
{
    AllocateMemoryParams allocParams;
    FreeMemoryParams freeParams;

    allocParams.nonPagedMemorySizeInBytes = nonPagedMemorySizeInBytes;
    allocParams.nonPagedMemoryAddress = NULL;
    allocParams.contiguousMemorySizeInBytes = contiguousMemorySizeInBytes;
    allocParams.numContiguousMemoryChunks = numContiguousMemoryChunks;
    allocParams.contiguousMemoryAddressesUserPtr = calloc(numContiguousMemoryChunks, sizeof(VIRTUAL_ADDRESS));    
    allocParams.elapsedTimeInMicroseconds = 0;    

    if (ERROR_IS_FATAL(error)) return allocParams.elapsedTimeInMicroseconds;

    DEBUG_PRINT
    ((
        "...allocating %Id bytes of nonpaged memory and %Id %Id-byte chunks of contiguous memory...\n", 
        allocParams.nonPagedMemorySizeInBytes,
        allocParams.numContiguousMemoryChunks,
        allocParams.contiguousMemorySizeInBytes
    ));

    allocateMemoryInKernel(device, &allocParams, error);
    
    freeParams.nonPagedMemoryAddress = allocParams.nonPagedMemoryAddress;
    freeParams.numContiguousMemoryChunks = allocParams.numContiguousMemoryChunks;
    freeParams.contiguousMemoryAddressesUserPtr = allocParams.contiguousMemoryAddressesUserPtr;

    DEBUG_PRINT
    ((
        "...freeing memory...\n"
    ));
        
    freeMemoryInKernel(device, &freeParams, error);    

    return allocParams.elapsedTimeInMicroseconds;
}

VOID __cdecl runTests
(
    HANDLE device, 
    SIZE_T nonPagedMemorySizeInBytes, 
    SIZE_T contiguousMemorySizeInBytes,
    SIZE_T numContiguousMemoryChunks,
    SIZE_T numTests,
    ERR* error
)
{
    if (ERROR_IS_FATAL(error)) return;

    printf("Running %Id tests...\n\n", numTests);
    
    UINT64 totalTestTime = 0;

    for (SIZE_T i = 0; i < numTests && !ERROR_IS_FATAL(error); i++)
    {
        DEBUG_PRINT(("\n\nRunning test[%Id]...\n", i));
        UINT64 elapsedTimeInMicroseconds = runTest
        (
            device, 
            nonPagedMemorySizeInBytes,
            contiguousMemorySizeInBytes,
            numContiguousMemoryChunks,    
            error
        );
        printf("Test[%Id] took [%lld] microseconds\n", i, elapsedTimeInMicroseconds);
        totalTestTime += elapsedTimeInMicroseconds;
    }
    printf("\nAverage Test Time (microseconds): %lld\n", totalTestTime / numTests);
}


VOID __cdecl
main(_In_ ULONG argc, _In_reads_(argc) PCHAR argv[])
{
    TCHAR driverLocation[MAX_PATH];
    HANDLE device = INVALID_HANDLE_VALUE;
    ERR error = NOERROR;

    ULONG argIndex;
    SIZE_T numTests;
    SIZE_T nonPagedMemorySizeInBytes;
    SIZE_T contiguousMemorySizeInBytes;
    SIZE_T numContiguousMemoryChunks;

    argc -= 1;
    argIndex = 0;

    if (argc >= ++argIndex)
    {
        numTests = atoi(argv[argIndex]);
    }
    else
    {
        printf("Usage: %s NUM_TESTS [NON_PAGED_MEMORY_SIZE_IN_BYTES] [CONTIGUOUS_MEMORY_SIZE_IN_BYTES] [NUM_CONTIGUOUS_MEMORY_CHUNKS]\n", argv[0]);
        return;        
    }

    if (argc >= ++argIndex)
    {
        nonPagedMemorySizeInBytes = atoi(argv[argIndex]);
    }
    else
    {
        nonPagedMemorySizeInBytes = 14400000;
    }

    if (argc >= ++argIndex)
    {
        contiguousMemorySizeInBytes = atoi(argv[argIndex]);
    }
    else
    {
        contiguousMemorySizeInBytes = 1023;
    }

    if (argc >= ++argIndex)
    {
        numContiguousMemoryChunks = atoi(argv[argIndex]);
    }
    else
    {
        numContiguousMemoryChunks = 3500;
    }
    
    if (!SetupDriverLocation(DRIVER_NAME, driverLocation, sizeof(driverLocation))) return;
    openDeviceHandle(driverLocation, &device, &error);
    
    runTests
    (
        device, 
        nonPagedMemorySizeInBytes,
        contiguousMemorySizeInBytes,
        numContiguousMemoryChunks,
        numTests, 
        &error
    );

    CloseHandle(device);

    //
    // Unload the driver.  Ignore any errors.
    //

    ManageDriver(DRIVER_NAME,
        driverLocation,
        DRIVER_FUNC_REMOVE
    );

    if (ERROR_IS_FATAL(&error))
    {
        printf("Program exited with fatal error: %d\n", error);
    }
}


