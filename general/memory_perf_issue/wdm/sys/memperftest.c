/*
    Demonstrates a memory performance issue in MmallocateMemory
*/

//
// Include files.
//

#include <ntddk.h>
#include <wdm.h>
#include <string.h>

#include "memperftest.h"
#include "userbuffer.h"
#include "types.h"

#define NT_DEVICE_NAME L"\\Device\\MemPerfTestDevice"
#define DOS_DEVICE_NAME L"\\DosDevices\\MemPerfTestDevice"

#if DBG
#define MEMPERFTEST_KDPRINT(_x_) \
   DbgPrint("MEMPERFTEST.SYS: "); \
   DbgPrint _x_;

#else
#define MEMPERFTEST_KDPRINT(_x_)
#endif

//
// Device driver routine declarations.
//

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH MemPerfTestCreateClose; 

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH MemPerfTestDeviceControl;

DRIVER_UNLOAD MemPerfTestUnloadDriver;


//
// Helper functions
//

NTSTATUS
onAllocateMemory
(
    PIRP irp,
    PIO_STACK_LOCATION irpSp
);

NTSTATUS
allocateMemory
(
    AllocateMemoryParams* params
);

NTSTATUS
onFreeMemory
(
    PIRP irp,
    PIO_STACK_LOCATION irpSp
);

NTSTATUS
freeMemory
(
    FreeMemoryParams* params
);

VOID
printIrpInfo
(
    PIRP Irp
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry )
#pragma alloc_text(PAGE, MemPerfTestCreateClose)
#pragma alloc_text(PAGE, MemPerfTestDeviceControl)
#pragma alloc_text(PAGE, MemPerfTestUnloadDriver)
#pragma alloc_text(PAGE, onAllocateMemory)
#pragma alloc_text(PAGE, allocateMemory)
#pragma alloc_text(PAGE, onFreeMemory)
#pragma alloc_text(PAGE, freeMemory)
#pragma alloc_text(PAGE, printIrpInfo)
#endif


/*
    Routine Description:
        This routine is called by the Operating System to initialize the driver.
        It creates the device object, fills in the dispatch entry points and
            completes the initialization.

    Arguments:
        DriverObject - a pointer to the object that represents this device
            driver.
        RegistryPath - a pointer to our Services key in the registry.

    Return Value:
        STATUS_SUCCESS if initialized; an error otherwise.
*/
NTSTATUS
DriverEntry
(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS ntStatus;
    UNICODE_STRING ntUnicodeString; // NT Device Name
    UNICODE_STRING ntWin32NameString; // Win32 Name
    PDEVICE_OBJECT deviceObject = NULL;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&ntUnicodeString, NT_DEVICE_NAME);

    ntStatus = IoCreateDevice
    (
        DriverObject,                   
        0,                              // We don't use a device extension
        &ntUnicodeString, 
        FILE_DEVICE_UNKNOWN,            // Device type
        FILE_DEVICE_SECURE_OPEN,        // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject 
    );          

    if (!NT_SUCCESS(ntStatus))
    {
        MEMPERFTEST_KDPRINT(("Couldn't create the device object\n"));
        return ntStatus;
    }

    
    // Initialize the driver object with this driver's entry points.    
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MemPerfTestCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MemPerfTestCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MemPerfTestDeviceControl;
    DriverObject->DriverUnload = MemPerfTestUnloadDriver;
        
    RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);

    ntStatus = IoCreateSymbolicLink(&ntWin32NameString, &ntUnicodeString);

    if (!NT_SUCCESS(ntStatus))
    {
        MEMPERFTEST_KDPRINT(("Couldn't create symbolic link\n"));
        IoDeleteDevice(deviceObject);
    }

    return ntStatus;
}

/*
    Routine Description:
        This routine is called by the I/O system when the MEMPERFTEST is opened or
            closed.
        No action is performed other than completing the request successfully.

    Arguments:
        deviceObject - a pointer to the object that represents the device
            that I/O is to be done on.
        irp - a pointer to the I/O Request Packet for this request.

    Return Value:
        NT status code
*/
NTSTATUS
MemPerfTestCreateClose
(
    PDEVICE_OBJECT deviceObject,
    PIRP irp
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    PAGED_CODE();

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


/*
    Routine Description:
        This routine is called by the I/O system to unload the driver.
        Any resources previously allocated must be freed.

    Arguments:
        driverObject - a pointer to the object that represents our driver.

    Return Value:
        None
*/
VOID
MemPerfTestUnloadDriver
(
    _In_ PDRIVER_OBJECT driverObject
)
{
    PDEVICE_OBJECT deviceObject = driverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    PAGED_CODE();

    RtlInitUnicodeString(&uniWin32NameString, DOS_DEVICE_NAME);
        
    IoDeleteSymbolicLink(&uniWin32NameString);

    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }
}

NTSTATUS
MemPerfTestDeviceControl
(
    PDEVICE_OBJECT deviceObject,
    PIRP irp
)
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG controlCode;            

    UNREFERENCED_PARAMETER(deviceObject);

    PAGED_CODE();
        
    irpSp = IoGetCurrentIrpStackLocation(irp);
    controlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;

    MEMPERFTEST_KDPRINT(("Called control %d.\n", controlCode));

    switch (controlCode)
    {
        case IOCTL_MEMPERFTEST_ALLOCATEMEMORY:
            ntStatus = onAllocateMemory(irp, irpSp);
            break;
        case IOCTL_MEMPERFTEST_FREEMEMORY:
            ntStatus = onFreeMemory(irp, irpSp);
            break;
        default:
            MEMPERFTEST_KDPRINT(("Error: unexpected IoControlCode encountered: %ld", controlCode));
            printIrpInfo(irp);
            break;
    }
    
    irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return ntStatus;
}

NTSTATUS
onAllocateMemory
(
    PIRP irp,
    PIO_STACK_LOCATION irpSp
)
{    
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SIZE_T inputBufferLength;
    SIZE_T outputBufferLength;

    AllocateMemoryParams* params;

    inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
    PAGED_CODE();
    
    if 
    (
        inputBufferLength < sizeof(AllocateMemoryParams) ||
        outputBufferLength < sizeof(AllocateMemoryParams)
    )
    {
        MEMPERFTEST_KDPRINT(("Error: Unexpected input or output buffer size.\n"));
        MEMPERFTEST_KDPRINT(("Actual Sizes: input(%Id) output(%Id).\n", inputBufferLength, outputBufferLength));
        MEMPERFTEST_KDPRINT(("Expected Minimum Sizes: input(%Id) output(%Id).\n", sizeof(AllocateMemoryParams), sizeof(AllocateMemoryParams)));
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }
    
    params = (AllocateMemoryParams*)irp->AssociatedIrp.SystemBuffer;

    if (!params)
    {
        MEMPERFTEST_KDPRINT(("Error: NULL input encountered.\n"));
        MEMPERFTEST_KDPRINT(("params address: %p.\n", params));
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    ntStatus = allocateMemory(params);
    if (!NT_SUCCESS(ntStatus))
    {
        goto End;
    }
            
    irp->IoStatus.Information = sizeof(AllocateMemoryParams);
    
End:
    return ntStatus;
}

NTSTATUS
allocateMemory
(
    AllocateMemoryParams* params
)
{    
    LARGE_INTEGER startingTime;
    LARGE_INTEGER endingTime;
    LARGE_INTEGER performanceFrequency;    

    NTSTATUS ntStatus = STATUS_SUCCESS;    
    const PHYSICAL_ADDRESS highestAcceptableAddress = { MAXULONG64 };
    const PHYSICAL_ADDRESS zeroPhysicalAddress = { 0ll };

    SIZE_T i;
    UserBuffer userBuffer;
    VIRTUAL_ADDRESS* contiguousMemoryAddresses;

    ntStatus = initializeUserBuffer(params->contiguousMemoryAddressesUserPtr, params->numContiguousMemoryChunks * sizeof(VIRTUAL_ADDRESS), &userBuffer);
    if (!NT_SUCCESS(ntStatus))
    {
        goto End;
    }

    contiguousMemoryAddresses = (VIRTUAL_ADDRESS*)userBuffer.data;

    startingTime = KeQueryPerformanceCounter(&performanceFrequency);

    params->nonPagedMemoryAddress = ExAllocatePoolWithTag(NonPagedPool, params->nonPagedMemorySizeInBytes, 'freP');

    for (i = 0; i < params->numContiguousMemoryChunks; i++)
    {
        contiguousMemoryAddresses[i] = MmAllocateContiguousMemorySpecifyCache
        (
            params->contiguousMemorySizeInBytes,
            zeroPhysicalAddress, 
            highestAcceptableAddress, 
            zeroPhysicalAddress, 
            MmCached
        );
    }

    endingTime = KeQueryPerformanceCounter(NULL);

    params->elapsedTimeInMicroseconds = ((endingTime.QuadPart - startingTime.QuadPart) * 1000000) / performanceFrequency.QuadPart;

End:
    finalizeUserBuffer(&userBuffer);
    return ntStatus;
}

NTSTATUS
onFreeMemory
(
    PIRP irp,
    PIO_STACK_LOCATION irpSp
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG inputBufferLength;

    FreeMemoryParams* params;

    PAGED_CODE();

    inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (inputBufferLength < sizeof(FreeMemoryParams))
    {
        MEMPERFTEST_KDPRINT(("Error: Unexpected input buffer size.\n"));
        MEMPERFTEST_KDPRINT(("Actual Size: %Id.\n", inputBufferLength));
        MEMPERFTEST_KDPRINT(("Expected Minimum Size: %Id.\n", sizeof(FreeMemoryParams)));
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }
        
    params = (FreeMemoryParams*)irp->AssociatedIrp.SystemBuffer;

    if (!params)
    {
        MEMPERFTEST_KDPRINT(("Error: NULL input encountered.\n"));
        MEMPERFTEST_KDPRINT(("params address: %p.\n", params));
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    freeMemory(params);
    

End:
    return ntStatus;
}

NTSTATUS
freeMemory(FreeMemoryParams* params)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SIZE_T i;
    UserBuffer userBuffer;
    VIRTUAL_ADDRESS* contiguousMemoryAddresses;
    

    ntStatus = initializeUserBuffer(params->contiguousMemoryAddressesUserPtr, params->numContiguousMemoryChunks * sizeof(VIRTUAL_ADDRESS), &userBuffer);
    if (!NT_SUCCESS(ntStatus))
    {
        goto End;
    }

    contiguousMemoryAddresses = (VIRTUAL_ADDRESS*)userBuffer.data;

    for (i = params->numContiguousMemoryChunks; i != 0 ; i--)
    {
        MmFreeContiguousMemory(contiguousMemoryAddresses[i - 1]);
    }

    ExFreePool(params->nonPagedMemoryAddress);

End:
    finalizeUserBuffer(&userBuffer);
    return ntStatus;
}



VOID
printIrpInfo
(
    PIRP irp
)
{
    PIO_STACK_LOCATION  irpSp;
    irpSp = IoGetCurrentIrpStackLocation(irp);

    PAGED_CODE();

    MEMPERFTEST_KDPRINT
    ((
        "\tirp->AssociatedIrp.SystemBuffer = 0x%p\n",
        irp->AssociatedIrp.SystemBuffer
    ));

    MEMPERFTEST_KDPRINT(("\tirp->UserBuffer = 0x%p\n", irp->UserBuffer));

    MEMPERFTEST_KDPRINT
    ((
        "\tirpSp->Parameters.DeviceIoControl.Type3InputBuffer = 0x%p\n",
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer
    ));

    MEMPERFTEST_KDPRINT
    ((
        "\tirpSp->Parameters.DeviceIoControl.InputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.InputBufferLength
    ));

    MEMPERFTEST_KDPRINT
    ((
        "\tirpSp->Parameters.DeviceIoControl.OutputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.OutputBufferLength
    ));
}
