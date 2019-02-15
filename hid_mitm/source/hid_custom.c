
#include <stdio.h>
#include <switch.h>

static Service hid_service;
static Service hid_iappletresource = {0};
static SharedMemory hid_shmem;

//#define USE_REAL_APPLET_RESOURCE

static Result _customHidCreateAppletResource(Service *srv)
{
    /*
    What is happening here is that libstratosphere has the original request buffer (modified to spoof the pid) already in the tls so we can just execute serviceIpcDispatch and be happy.
    This will *only* work if it's the first ipc done after getting the mitm-call.
    */

    #ifndef USE_REAL_APPLET_RESOURCE
   

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->AppletResourceUserId = 0;
    #endif


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
            serviceCreate(&hid_iappletresource, r.Handles[0]);
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

bool initialized = false;
void customHidInitialize(Service* serv)
{
    if(initialized)
        return;
    initialized = true;


    Result rc;
    rc = _customHidCreateAppletResource(serv); // Executes the original ipc
    if (R_FAILED(rc))
        fatalSimple(rc);

    hid_service = *serv;

    Handle sharedMemHandle;

    rc = _customHidGetSharedMemoryHandle(&hid_iappletresource, &sharedMemHandle);
    if (R_FAILED(rc))
        fatalSimple(rc);

    shmemLoadRemote(&hid_shmem, sharedMemHandle, 0x40000, Perm_R);
    rc = shmemMap(&hid_shmem);
    if (R_FAILED(rc))
        fatalSimple(rc);
}