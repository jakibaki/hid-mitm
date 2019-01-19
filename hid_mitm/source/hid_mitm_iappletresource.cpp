
#include <mutex>
#include <switch.h>
#include "hid_mitm_iappletresource.hpp"

static SharedMemory fake_shmem = {0};
static HidSharedMemory* fake_shmem_mem;
static HidSharedMemory* real_shmem_mem;
static HidSharedMemory tmp_shmem_mem;

static Thread shmem_patch_thread;

void copy_thread(void* _)
{
    while(true)
    {
        svcSleepThread(5000000);
        tmp_shmem_mem = *real_shmem_mem;
        for(int i = 0; i < 10; i++) {
            for(int l = 0; l < 7; l++) {
                HidControllerLayout *currentLayout = &tmp_shmem_mem.controllers[i].layouts[l];
                for(int j = 0; j < 17; j++) {
                    HidControllerInputEntry* curEnt = &currentLayout->entries[j];
                    int a_held = curEnt->buttons & KEY_A;
                    int b_held = curEnt->buttons & KEY_B;
                    if(a_held) {
                        curEnt->buttons |= KEY_B;
                    } else {
                        curEnt->buttons &= ~KEY_B;
                    }

                    if(b_held) {
                        curEnt->buttons |= KEY_A;
                    } else {
                        curEnt->buttons &= ~KEY_A;
                    }
                }

            }
        }
        *fake_shmem_mem = tmp_shmem_mem;
    }
}

Result IAppletResourceMitmService::GetSharedMemoryHandle(Out<CopiedHandle> shmem_hand)
{
    if(fake_shmem.handle == NULL) {
        shmemCreate(&fake_shmem, sizeof(HidSharedMemory), Perm_Rw, Perm_R);
        shmemMap(&fake_shmem);
        fake_shmem_mem = (HidSharedMemory*) shmemGetAddr(&fake_shmem);
        real_shmem_mem = (HidSharedMemory*) hidGetSharedmemAddr();
        *fake_shmem_mem = *real_shmem_mem;
        threadCreate(&shmem_patch_thread, copy_thread, NULL, 0x1000, 0x3F, 3);
        threadStart(&shmem_patch_thread);
    }
    


    //SharedMemory* shmem = hidGetSharedmem();

    shmem_hand.SetValue(fake_shmem.handle);



    return 0;
}

void IAppletResourceMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* No commands need postprocessing. */    
}