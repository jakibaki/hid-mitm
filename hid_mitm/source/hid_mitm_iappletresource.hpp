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


class IAppletResourceMitmService : public ams::sf::IServiceObject {
    public:
        IAppletResourceMitmService(IAppletResourceMitmService* dummy)  {
            /* ... */
        }
        
        ~IAppletResourceMitmService();

    protected:
        /* Overridden commands. */
        virtual ams::Result GetSharedMemoryHandle(ams::sf::OutCopyHandle shmem_hand) final;
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            ams::sf::MakeServiceCommandMeta<IAppletResource_GetSharedMemoryHandle, &ServiceImpl::GetSharedMemoryHandle>()
			// TODO: use MAKE_SERVICE_COMMAND_META(GetSharedMemoryHandle)
        };

    ams::os::ProcessId pid;
    Service iappletresource_handle;
    SharedMemory real_sharedmem;
    SharedMemory fake_sharedmem;
};

