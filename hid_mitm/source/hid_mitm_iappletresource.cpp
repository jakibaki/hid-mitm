
#include <mutex>
#include <map>
#include <switch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    // makes it so that when KEY_A is pressed we get KEY_B
    cfg[KEY_A] = KEY_B;

    // makes it so that when KEY_B is pressed we get KEY_A
    cfg[KEY_B] = KEY_A;
}

void rebind_keys(int gamepad_ind, int layout)
{
    HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[gamepad_ind].layouts[layout];
    HidControllerLayout *currentRealLayout = &real_shmem_mem->controllers[gamepad_ind].layouts[layout];

    int cur_ent = currentTmpLayout->header.latestEntry;

    HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_ent];
    HidControllerInputEntry *curRealEnt = &currentRealLayout->entries[cur_ent];

    for (auto const &[key, value] : rebind_config)
    {
        bool key_held = !!(curRealEnt->buttons & key);
        if (key_held)
            curTmpEnt->buttons |= value;
        else
            curTmpEnt->buttons &= ~value;
    }
}

void get_keys(int sock, int gamepad_ind)
{
    u64 buttons;
    recv(sock, &buttons, sizeof(u64), 0);

    for (int layout = 0; layout < 7; layout++)
    {
        HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[gamepad_ind].layouts[layout];
        HidControllerLayout *currentRealLayout = &real_shmem_mem->controllers[gamepad_ind].layouts[layout];

        int cur_ent = currentTmpLayout->header.latestEntry;

        HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_ent];
        HidControllerInputEntry *curRealEnt = &currentRealLayout->entries[cur_ent];
        curTmpEnt->buttons = buttons;
    }
}

void send_keys(int sock, int gamepad_ind)
{
    int layout = LAYOUT_HANDHELD; // TODO: May be a bad idea, not sure
    HidControllerLayout *currentTmpLayout = &tmp_shmem_mem.controllers[gamepad_ind].layouts[layout];
    HidControllerLayout *currentRealLayout = &real_shmem_mem->controllers[gamepad_ind].layouts[layout];

    int cur_ent = currentTmpLayout->header.latestEntry;

    HidControllerInputEntry *curTmpEnt = &currentTmpLayout->entries[cur_ent];
    HidControllerInputEntry *curRealEnt = &currentRealLayout->entries[cur_ent];
    send(sock, &curTmpEnt->buttons, sizeof(u64), 0);
}

void copy_thread(void *_)
{

    bool network_setup = false;
    int i = 0; // So we don't check for network 60 times/second
    int sock;

    u64 curTime = svcGetSystemTick();
    u64 oldTime;

    while (true)
    {
        curTime = svcGetSystemTick(); // TODO: I'm tired while writing this code, check if I'm being an idiot

        if (i % 60 == 0 && !network_setup && gethostid() != INADDR_LOOPBACK)
        {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in serv_addr = {0};
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(5555);
            inet_pton(AF_INET, "192.168.0.38", &serv_addr.sin_addr);
            connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            network_setup = true;
        }

        tmp_shmem_mem = *real_shmem_mem;
        if (network_setup)
        {
            //char buf[1000];
            //sprintf(buf, "%lx\n", svcGetSystemTick());
            //send(sock, buf, strlen(buf), 0);
            get_keys(sock, CONTROLLER_HANDHELD);
            //send_keys(sock, CONTROLLER_HANDHELD);
        }
        /*        for (int i = 0; i < 7; i++)
            rebind_keys(CONTROLLER_HANDHELD, i); // Handheld joycons
*/
        *fake_shmem_mem = tmp_shmem_mem;

        i++;

        oldTime = curTime;
        curTime = svcGetSystemTick();
        svcSleepThread(std::max(1000L, (s64) (16666666 - ((curTime - oldTime) * 1e+9L / 19200000))));
    }
}

Result IAppletResourceMitmService::GetSharedMemoryHandle(Out<CopiedHandle> shmem_hand)
{
    if (fake_shmem.handle == 0)
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