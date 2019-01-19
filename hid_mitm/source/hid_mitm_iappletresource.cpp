
#include <mutex>
#include <map>
#include <switch.h>
#include "hid_mitm_iappletresource.hpp"

static SharedMemory fake_shmem = {0};
static HidSharedMemory *fake_shmem_mem;
static HidSharedMemory *real_shmem_mem;
static HidSharedMemory tmp_shmem_mem;

static Thread shmem_patch_thread;

std::map<u64, u64> rebind_config;
// VALUE is the key that we want to get when we click KEY

void loadConfig(std::map<u64, u64> &cfg)
{
    cfg.clear();
    /*
    // makes it so that when KEY_A is pressed we get KEY_B
    cfg[KEY_A] = KEY_B;

    // makes it so that when KEY_B is pressed we get KEY_A
    cfg[KEY_B] = KEY_A;
    */
}

void copy_thread(void *_)
{
    u64 curTime = svcGetSystemTick();
    u64 oldTime;
    while (true)
    {
        oldTime = curTime;
        curTime = svcGetSystemTick();

        svcSleepThread(std::max(1000L, 16666666 - (s64) ((curTime - oldTime) * 0.0192)));

        tmp_shmem_mem = *real_shmem_mem;
        for (int i = 0; i < 10; i++)
        {
            for (int l = 0; l < 7; l++)
            {
                HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[i].layouts[l];
                HidControllerLayout *currentRealLayout = &real_shmem_mem->controllers[i].layouts[l];
                for (int j = 0; j < 17; j++)
                {
                    HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[j];
                    HidControllerInputEntry *curRealEnt = &currentRealLayout->entries[j];

                    for(auto const& [key, value]: rebind_config)
                    {
                        bool key_held = !!(curRealEnt->buttons & key);
                        if(key_held)
                            curTmpEnt->buttons |= value;
                        else
                            curTmpEnt->buttons &= ~value;
                    }
                }
            }
        }
        *fake_shmem_mem = tmp_shmem_mem;
    }
}

Result IAppletResourceMitmService::GetSharedMemoryHandle(Out<CopiedHandle> shmem_hand)
{
    if (fake_shmem.handle == NULL)
    {
        shmemCreate(&fake_shmem, sizeof(HidSharedMemory), Perm_Rw, Perm_R);
        shmemMap(&fake_shmem);
        fake_shmem_mem = (HidSharedMemory *)shmemGetAddr(&fake_shmem);
        real_shmem_mem = (HidSharedMemory *)hidGetSharedmemAddr();

        // Copy over for the first time to make sure that there never is a "bad" sharedmem to be seen
        *fake_shmem_mem = *real_shmem_mem;
        loadConfig(rebind_config);
        threadCreate(&shmem_patch_thread, copy_thread, NULL, 0x1000, 0x2C, 3);
        threadStart(&shmem_patch_thread);
    }

    shmem_hand.SetValue(fake_shmem.handle);

    return 0;
}

void IAppletResourceMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx)
{
    /* No commands need postprocessing. */
}