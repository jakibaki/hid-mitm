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

static SharedMemory fake_shmem = {0};
static HidSharedMemory *fake_shmem_mem;
static HidSharedMemory *real_shmem_mem;
static HidSharedMemory tmp_shmem_mem;

static Thread shmem_patch_thread;

static std::unordered_map<u64, u64> rebind_config;
// VALUE is the key that we want to get when we click KEY

static Mutex configMutex;

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
    rebind_config[key] = val;
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
        if(rebind_config.size() > 0) {
            u64 buttons = 0;

            for (auto it = rebind_config.begin(); it != rebind_config.end(); it++)
            {
                if (curTmpEnt->buttons & it->first)
                {
                    buttons |= it->second;
                }
            }
            //buttons |= curTmpEnt->buttons & (KEY_JOYCON_LEFT | KEY_JOYCON_DOWN | KEY_JOYCON_RIGHT | KEY_JOYCON_UP | KEY_LSTICK_DOWN | KEY_LSTICK_RIGHT | KEY_LSTICK_LEFT | KEY_LSTICK_UP | KEY_RSTICK_DOWN | KEY_RSTICK_LEFT | KEY_RSTICK_RIGHT | KEY_RSTICK_UP);
            curTmpEnt->buttons = buttons;
            
        }
        mutexUnlock(&configMutex);
    }
}

void get_from_udp() {
    struct input_msg msg;
    int res = poll_udp_input(&msg);
    if(res != 0)
        return;
    
    for (int layout = 0; layout < LAYOUT_DEFAULT + 1; layout++)
    {
        u32 controllers[] = {CONTROLLER_HANDHELD, CONTROLLER_PLAYER_1};

        for(u32 gamepad = 0; gamepad < sizeof(controllers)/ sizeof(u32); gamepad++) {
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

void copy_thread(void *_)
{
    u64 curTime = svcGetSystemTick();
    u64 oldTime;

    while (true)
    {
        curTime = svcGetSystemTick();

        tmp_shmem_mem = *real_shmem_mem;

        get_from_udp();
        rebind_keys(CONTROLLER_HANDHELD);
        rebind_keys(CONTROLLER_PLAYER_1);

        *fake_shmem_mem = tmp_shmem_mem;

        oldTime = curTime;
        curTime = svcGetSystemTick();
        svcSleepThread(std::max(1000L, (s64)(16666666 - ((curTime - oldTime) * 1e+9L / 19200000))));
    }
}

Result IAppletResourceMitmService::GetSharedMemoryHandle(Out<CopiedHandle> shmem_hand)
{
    if (fake_shmem.handle == 0)
    {
        mutexInit(&configMutex);
        shmemCreate(&fake_shmem, sizeof(HidSharedMemory), Perm_Rw, Perm_R);
        shmemMap(&fake_shmem);
        fake_shmem_mem = (HidSharedMemory *)shmemGetAddr(&fake_shmem);
        real_shmem_mem = (HidSharedMemory *)customHidGetSharedmemAddr();


        // Copy over for the first time to make sure that there never is a "bad" sharedmem to be seen
        *fake_shmem_mem = *real_shmem_mem;
        loadConfig();
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