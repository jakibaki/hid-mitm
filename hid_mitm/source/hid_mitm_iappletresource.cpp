#include <mutex>
#include <map>
#include <switch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "ini.h"
#include "hid_custom.h"
#include "udp_input.h"

#include "hid_mitm_iappletresource.hpp"

//static SharedMemory fake_shmem = {0};
//static HidSharedMemory *fake_shmem_mem;
//static HidSharedMemory *real_shmem_mem;

static std::unordered_map<u64, std::pair<HidSharedMemory *, HidSharedMemory *>> sharedmems;

static HidSharedMemory tmp_shmem_mem;

static Thread shmem_patch_thread;

//static std::unordered_map<u64, u64> rebind_config;
static std::vector<std::pair<u64, u64>> rebind_config;
// VALUE is the key that we want to get when we click KEY

static Mutex configMutex;

void add_shmem(u64 pid, SharedMemory* real_shmem, SharedMemory* fake_shmem)
{
    mutexLock(&shmem_mutex);
    HidSharedMemory* real_mapped = (HidSharedMemory*) shmemGetAddr(real_shmem);
    HidSharedMemory* fake_mapped = (HidSharedMemory*) shmemGetAddr(fake_shmem);
    sharedmems[pid] = std::pair<HidSharedMemory*, HidSharedMemory*>(real_mapped, fake_mapped);
    mutexUnlock(&shmem_mutex);
}

void del_shmem(u64 pid)
{
    mutexLock(&shmem_mutex);
    if(sharedmems.find(pid) != sharedmems.end()) {
        sharedmems.erase(pid);
    }
    mutexUnlock(&shmem_mutex);
}

std::string key_names[] = {"KEY_A", "KEY_B", "KEY_X", "KEY_Y", "KEY_LSTICK", "KEY_RSTICK", "KEY_L", "KEY_R", "KEY_ZL", "KEY_ZR", "KEY_PLUS", "KEY_MINUS", "KEY_DLEFT", "KEY_DUP", "KEY_DRIGHT", "KEY_DDOWN"};

s64 get_key_ind(std::string str)
{
    for (u64 i = 0; i < sizeof(key_names) / sizeof(char *); i++)
    {
        if (str == key_names[i])
        {
            return BIT(i);
        }
    }
    return -1;
}

/* sample config:
; Gamepad-rebind config. Currently only supports player one rebinding.
; VALUE is the button that gets registered when KEY is held down
[player1]
KEY_A = KEY_A
KEY_B = KEY_B
KEY_X = KEY_X
KEY_Y = KEY_Y
KEY_LSTICK = KEY_LSTICK
KEY_RSTICK = KEY_RSTICK
KEY_L = KEY_L
KEY_R = KEY_R
KEY_ZL = KEY_ZL
KEY_ZR = KEY_ZR
KEY_PLUS = KEY_PLUS
KEY_MINUS = KEY_MINUS
KEY_DLEFT = KEY_DLEFT
KEY_DUP = KEY_DUP
KEY_DRIGHT = KEY_DRIGHT
KEY_DDOWN = KEY_DDOWN
*/

static int handler(void *dummy, const char *section, const char *name,
                   const char *value)
{
    s64 key = get_key_ind(name);
    s64 val = get_key_ind(value);

    if (key < 0 || val < 0)
    {
        return -1;
    }
    rebind_config.push_back(std::pair<u64, u64>(key, val));
    return 0;
}

void loadConfig()
{
    mutexLock(&configMutex);
    rebind_config.clear();
    if (ini_parse("/modules/hid_mitm/config.ini", handler, NULL) < 0)
    {
        //fatalSimple(MAKERESULT(321, 1)); // 2321-0001 bad config
    }
    mutexUnlock(&configMutex);
}

// Clears config until next load
void clearConfig()
{
    mutexLock(&configMutex);
    rebind_config.clear();
    mutexUnlock(&configMutex);
}

void rebind_keys(int gamepad_ind)
{
    for (int layout = 0; layout < LAYOUT_DEFAULT + 1; layout++)
    {
        HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[gamepad_ind].layouts[layout];

        int cur_tmp_ent_ind = currentTmpLayout->header.latestEntry;

        HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_tmp_ent_ind];

        mutexLock(&configMutex);
        if (rebind_config.size() > 0)
        {
            u64 buttons = 0;

            for (auto it = rebind_config.begin(); it != rebind_config.end(); it++)
            {
                if (curTmpEnt->buttons & it->first)
                {
                    buttons |= it->second;
                }
            }

            // lstick rstick digital stuff
            for(int i = 16; i <= 27; i++) {
                buttons |= curTmpEnt->buttons & BIT(i);
            }
            curTmpEnt->buttons = buttons;
        }
        mutexUnlock(&configMutex);
    }
}

void apply_fake_gamepad(struct input_msg msg)
{

    for (int layout = 0; layout < LAYOUT_DEFAULT + 1; layout++)
    {
        u32 controllers[] = {CONTROLLER_HANDHELD, CONTROLLER_PLAYER_1};

        for (u32 gamepad = 0; gamepad < sizeof(controllers) / sizeof(u32); gamepad++)
        {
            HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[controllers[gamepad]].layouts[layout];
            int cur_tmp_ent_ind = currentTmpLayout->header.latestEntry;
            HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_tmp_ent_ind];
            curTmpEnt->buttons = msg.keys;
            curTmpEnt->joysticks[0].dx = msg.joy_l_x;
            curTmpEnt->joysticks[0].dy = msg.joy_l_y;
            curTmpEnt->joysticks[1].dx = msg.joy_r_x;
            curTmpEnt->joysticks[1].dy = msg.joy_r_y;
        }
    }
}

void shmem_copy(HidSharedMemory* source, HidSharedMemory* dest)
{
    memcpy(dest->controllers, source->controllers, sizeof(dest->controllers));

    // Apparently unused
    //memcpy(dest->controllerSerials, source->controllerSerials, sizeof(dest->controllerSerials));
    dest->keyboard = source->keyboard;
    dest->mouse = source->mouse;
    dest->touchscreen = source->touchscreen;
}

void copy_thread(void *_)
{

    u64 curTime = svcGetSystemTick();
    u64 oldTime;

    loadConfig();

    struct input_msg msg;
    while (true)
    {
        curTime = svcGetSystemTick();

        int poll_res = poll_udp_input(&msg);

        mutexLock(&shmem_mutex);
        for (auto it = sharedmems.begin(); it != sharedmems.end(); it++)
        {
            // TODO: Blacklist magic

            shmem_copy(it->second.first, &tmp_shmem_mem);

            if(poll_res == 0)
                apply_fake_gamepad(msg);

            rebind_keys(CONTROLLER_HANDHELD);
            rebind_keys(CONTROLLER_PLAYER_1);

            shmem_copy(&tmp_shmem_mem, it->second.second);
        }
        mutexUnlock(&shmem_mutex);

        oldTime = curTime;
        curTime = svcGetSystemTick();
        //svcSleepThread(std::max(1000L, (s64)(16666666 - ((curTime - oldTime) * 1e+9L / 19200000))));
        svcSleepThread(1e+9L / 60);
    }
}

void copyThreadInitialize()
{
    mutexInit(&configMutex);
    loadConfig();
    threadCreate(&shmem_patch_thread, copy_thread, NULL, 0x1000, 0x3E, 3);
    threadStart(&shmem_patch_thread);
}

IAppletResourceMitmService::~IAppletResourceMitmService() {
    del_shmem(this->pid);
    customHidExit(&this->iappletresource_handle, &this->real_sharedmem, &this->fake_sharedmem);
}


Result IAppletResourceMitmService::GetSharedMemoryHandle(Out<CopiedHandle> shmem_hand)
{
    shmem_hand.SetValue(this->fake_sharedmem.handle);

    return 0;
}

void IAppletResourceMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx)
{
    /* No commands need postprocessing. */
}