
#include <stdio.h>
#include <switch.h>

static Service hid_service;
static Service hid_iappletresource;
static SharedMemory hid_shmem;

static Result _customHidCreateAppletResource(Service *srv, Service *srv_out, u64 AppletResourceUserId, u32 pid)
{
    IpcCommand c;
    ipcInitialize(&c);
    struct
    {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } * raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    //TODO: Doesn't work yet
    AppletResourceUserId = 0;

    //u32 *cmdbuf = (u32 *)armGetTls();
    //cmdbuf[4] = 0xFFFE0000UL | (pid & 0xFFFFUL); // Makes sure that the command is sent with the fake-pid

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->AppletResourceUserId = AppletResourceUserId;
    
    
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
            serviceCreate(srv_out, r.Handles[0]);
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

void *customHidGetSharedmemAddr()
{
    return shmemGetAddr(&hid_shmem);
}

void customHidInitialize(u64 aruid, u64 pid)
{
    Result rc;
    rc = smGetService(&hid_service, "hid");
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = _customHidCreateAppletResource(&hid_service, &hid_iappletresource, aruid, pid);
    if (R_FAILED(rc))
        fatalSimple(rc);

    Handle sharedMemHandle;

    rc = _customHidGetSharedMemoryHandle(&hid_iappletresource, &sharedMemHandle);
    if (R_FAILED(rc))
        fatalSimple(rc);

    shmemLoadRemote(&hid_shmem, sharedMemHandle, 0x40000, Perm_R);
    rc = shmemMap(&hid_shmem);
    if (R_FAILED(rc))
        fatalSimple(rc);
}