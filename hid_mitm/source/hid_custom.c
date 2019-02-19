
#include <stdio.h>
#include <switch.h>
#include "hid_custom.h"

Mutex shmem_mutex;

static Result _customHidCreateAppletResource(Service *srv, Service *out_iappletresource)
{
    /*
    What is happening here is that libstratosphere has the original request buffer (modified to spoof the pid) already in the tls so we can just execute serviceIpcDispatch and be happy.
    This will *only* work if it's the first ipc done after getting the mitm-call.
    */

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc))
    {
        IpcParsedCommand r;
        ipcParse(&r);

        struct
        {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
        {
            serviceCreate(out_iappletresource, r.Handles[0]);
        }
    }

    return rc;
}

static Result _customHidGetSharedMemoryHandle(Service *srv, Handle *handle_out)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct
    {
        u64 magic;
        u64 cmd_id;
    } * raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc))
    {
        IpcParsedCommand r;
        ipcParse(&r);

        struct
        {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
        {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

void customHidSetup(Service* hid_service, Service* out_iappletresource, SharedMemory* out_real_shmem, SharedMemory* out_fake_shmem)
{
    Result rc;
    rc = _customHidCreateAppletResource(hid_service, out_iappletresource); // Executes the original ipc
    if (R_FAILED(rc))
        fatalSimple(rc);

    Handle sharedMemHandle;

    rc = _customHidGetSharedMemoryHandle(out_iappletresource, &sharedMemHandle);
    if (R_FAILED(rc))
        fatalSimple(rc);

    shmemLoadRemote(out_real_shmem, sharedMemHandle, 0x40000, Perm_R);
    rc = shmemMap(out_real_shmem);
    if (R_FAILED(rc))
        fatalSimple(rc);

    shmemCreate(out_fake_shmem, sizeof(HidSharedMemory), Perm_Rw, Perm_R);
    shmemMap(out_fake_shmem);

}

void customHidExit(Service* iappletresource, SharedMemory* real_shmem, SharedMemory* fake_shmem) 
{
    mutexLock(&shmem_mutex);
    shmemUnmap(real_shmem);
    shmemClose(real_shmem);

    shmemUnmap(fake_shmem);
    shmemClose(fake_shmem);
    mutexUnlock(&shmem_mutex);

    serviceClose(iappletresource);
}

void customHidInitialize()
{
    mutexInit(&shmem_mutex);
}