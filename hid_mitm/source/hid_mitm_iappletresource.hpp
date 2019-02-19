#pragma once
#include <switch.h>
#include <stratosphere.hpp>


void loadConfig();
void clearConfig();
void copyThreadInitialize();
void add_shmem(u64 pid, SharedMemory* real_shmem, SharedMemory* fake_shmem);
void del_shmem(u64 pid);

enum IAppletResourceCmd : u32 {
    IAppletResource_GetSharedMemoryHandle = 0
};


class IAppletResourceMitmService : public IServiceObject {      
    public:
        IAppletResourceMitmService(IAppletResourceMitmService* dummy)  {
            /* ... */
        }
        
        ~IAppletResourceMitmService();


        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);
        
    protected:
        /* Overridden commands. */
        virtual Result GetSharedMemoryHandle(Out<CopiedHandle> shmem_hand) final;
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<IAppletResource_GetSharedMemoryHandle, &IAppletResourceMitmService::GetSharedMemoryHandle>()
        };

    u64 pid;
    Service iappletresource_handle;
    SharedMemory real_sharedmem;
    SharedMemory fake_sharedmem;
};
