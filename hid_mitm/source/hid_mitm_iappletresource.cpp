#include <mutex>
#include <map>
#include <switch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "ini.h"
#include "hid_custom.h"

#include "hid_mitm_iappletresource.hpp"

static SharedMemory fake_shmem = {0};
static HidSharedMemory *fake_shmem_mem;
static HidSharedMemory *real_shmem_mem;
static HidSharedMemory tmp_shmem_mem;

static Thread shmem_patch_thread;

static std::unordered_map<u64, u64> rebind_config;
// VALUE is the key that we want to get when we click KEY

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
; KEY is the button that should register as pressed when VALUE is pressed
; Every single key should have a binding. Anything else is undefined behaviour
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
    rebind_config.clear();
    if (ini_parse("/modules/hid_mitm/config.ini", handler, NULL) < 0)
    {
        //fatalSimple(MAKERESULT(321, 1)); // 2321-0001 bad config
    }
}

void rebind_keys(int gamepad_ind)
{
    for (int layout = 0; layout < LAYOUT_DEFAULT + 1; layout++)
    {
        HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[gamepad_ind].layouts[layout];
        HidControllerLayout *currentRealLayout = &real_shmem_mem->controllers[gamepad_ind].layouts[layout];

        int cur_tmp_ent_ind = currentTmpLayout->header.latestEntry;
        int cur_real_ent_ind = currentRealLayout->header.latestEntry;

        HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_tmp_ent_ind];
        HidControllerInputEntry *curRealEnt = &currentRealLayout->entries[cur_real_ent_ind];

        for (auto it = rebind_config.begin(); it != rebind_config.end(); it++)
        {
            if (curRealEnt->buttons & it->second)
            {
                curTmpEnt->buttons |= it->first;
            }
            else
            {
                curTmpEnt->buttons &= ~it->first;
            }
        }
    }
}
int get_keys(int sock, int gamepad_ind)
{
    u64 buttons = 0;

    int ret = recv(sock, &buttons, sizeof(u64), 0);
    if (ret <= 0)
    {
        close(sock);
        return -1;
    }

    for (int layout = 0; layout < 7; layout++)
    {
        HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[gamepad_ind].layouts[layout];
        HidControllerLayout *currentRealLayout = &real_shmem_mem->controllers[gamepad_ind].layouts[layout];

        int cur_ent = currentTmpLayout->header.latestEntry;

        HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_ent];
        HidControllerInputEntry *curRealEnt = &currentRealLayout->entries[cur_ent];
        curTmpEnt->buttons = buttons;
    }
    return 0;
}

int send_keys(int sock, int gamepad_ind)
{
    int layout = LAYOUT_HANDHELD; // TODO: May be a bad idea, not sure
    HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[gamepad_ind].layouts[layout];
    HidControllerLayout *currentRealLayout = &real_shmem_mem->controllers[gamepad_ind].layouts[layout];

    int cur_ent = currentTmpLayout->header.latestEntry;

    HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_ent];
    HidControllerInputEntry *curRealEnt = &currentRealLayout->entries[cur_ent];
    if (send(sock, &curTmpEnt->buttons, sizeof(u64), 0) <= 0)
    {
        close(sock);
        return -1;
    }
    return 0;
}

void copy_thread(void *_)
{
    u64 curTime = svcGetSystemTick();
    u64 oldTime;

    while (true)
    {
        curTime = svcGetSystemTick();

        tmp_shmem_mem = *real_shmem_mem;

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
        
        customHidInitialize(aruid, pid);//((u32*) ((ServiceSession*) this)->backup_tls)[4]);

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