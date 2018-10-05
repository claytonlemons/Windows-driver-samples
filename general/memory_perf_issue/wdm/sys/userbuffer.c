#include "userbuffer.h"

NTSTATUS
initializeUserBuffer
(
    PVOID userPtr,
    SIZE_T bufferLength,
    UserBuffer* userBufferPtr
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG length;

    userBufferPtr->_initialized = FALSE;

    length = (ULONG)bufferLength;

    if (length < bufferLength)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    userBufferPtr->length = length;

    userBufferPtr->_mdl = IoAllocateMdl(userPtr, length, FALSE, TRUE, NULL);
    if (!userBufferPtr->_mdl)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    try
    {
        MmProbeAndLockPages(userBufferPtr->_mdl, UserMode, IoModifyAccess);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {

        ntStatus = GetExceptionCode();
        goto End;
    }


    userBufferPtr->data = MmGetSystemAddressForMdlSafe(userBufferPtr->_mdl, NormalPagePriority | MdlMappingNoExecute);

    if (!userBufferPtr->data)
    {
        MmUnlockPages(userBufferPtr->_mdl);
        IoFreeMdl(userBufferPtr->_mdl);
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    userBufferPtr->_initialized = TRUE;

End:
    return ntStatus;
}

void
finalizeUserBuffer
(
    UserBuffer* userBufferPtr
)
{
    if (userBufferPtr->_initialized)
    {
        MmUnlockPages(userBufferPtr->_mdl);
        IoFreeMdl(userBufferPtr->_mdl);
    };
}
