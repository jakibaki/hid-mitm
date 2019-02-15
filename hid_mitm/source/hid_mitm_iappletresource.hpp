#pragma once
#include <switch.h>
#include <stratosphere.hpp>


void loadConfig();
void clearConfig();

enum IAppletResourceCmd : u32 {
    IAppletResource_GetSharedMemoryHandle = 0
};


// TODO: Actually need to do the interface stuff since it expects the constructor to do stuff or something like that.s

class IAppletResourceMitmService : public IServiceObject {      
    public:
        IAppletResourceMitmService(IAppletResourceMitmService* dummy)  {
            /* ... */
        }
        


        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);
        
    protected:
        /* Overridden commands. */
        virtual Result GetSharedMemoryHandle(Out<CopiedHandle> shmem_hand) final;
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<IAppletResource_GetSharedMemoryHandle, &IAppletResourceMitmService::GetSharedMemoryHandle>()
        };
};
