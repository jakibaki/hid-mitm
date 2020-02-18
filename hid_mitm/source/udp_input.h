#ifdef __cplusplus
extern "C" {
#endif
#include <switch.h>

#define INPUT_MSG_MAGIC 0x3275

struct __attribute__((__packed__)) input_msg {
    u16 magic;
    u64 keys;
    s32 joy_l_x;
    s32 joy_l_y;
    s32 joy_r_x;
    s32 joy_r_y;
};


// Tries to get an input_msg through UDP. If it fails but didn't fail for the last 30 tries it will return a chached copy of the last successful poll
// Returns 0 on success -1 on faliure
int poll_udp_input(struct input_msg* buf);

#ifdef __cplusplus
}
#endif