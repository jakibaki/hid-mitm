from struct import pack, unpack
import sys
import os
import socket
from inputs import get_gamepad, devices
from time import sleep
import _thread

keys = {
    "A": 1,
    "B": 1 << 1,
    "X": 1 << 2,
    "Y": 1 << 3,
    "LST": 1 << 4,
    "RST": 1 << 5,
    "L": 1 << 6,
    "R": 1 << 7,
    "ZL": 1 << 8,
    "ZR": 1 << 9,
    "PLUS": 1 << 10,
    "MINUS": 1 << 11,
    "DL": 1 << 12,
    "DU": 1 << 13,
    "DR": 1 << 14,
    "DD": 1 << 15,
    "LL": 1 << 16,
    "LU": 1 << 17,
    "LR": 1 << 18,
    "LD": 1 << 19,
    "RL": 1 << 20,
    "RU": 1 << 21,
    "RR": 1 << 22,
    "RD": 1 << 23
}


def set_del_bit(bit, status, out):
    if status:
        out |= bit
    else:
        out &= ~bit
    return out


def check_keys(key, status, out):

    # for i in range(13):
    #     print(joystick.get_button(i))
    # print("--")

    if key == "BTN_NORTH":
        out = set_del_bit(keys["X"], status, out)
    if key == "BTN_EAST":
        out = set_del_bit(keys["A"], status, out)
    if key == "BTN_SOUTH":
        out = set_del_bit(keys["B"], status, out)
    if key == "BTN_WEST":
        out = set_del_bit(keys["Y"], status, out)

    if key == "BTN_DPAD_UP":
        out = set_del_bit(keys["DU"], status, out)
    if key == "BTN_DPAD_RIGHT":
        out = set_del_bit(keys["DR"], status, out)
    if key == "BTN_DPAD_DOWN":
        out = set_del_bit(keys["DD"], status, out)
    if key == "BTN_DPAD_LEFT":
        out = set_del_bit(keys["DL"], status, out)

    if key == "BTN_TL":
        out = set_del_bit(keys["L"], status, out)
    if key == "BTN_TR":
        out = set_del_bit(keys["R"], status, out)

    if key == "BTN_TL2":
        out = set_del_bit(keys["ZL"], status, out)
    if key == "BTN_TR2":
        out = set_del_bit(keys["ZR"], status, out)

    if key == "BTN_THUMBL":
        out = set_del_bit(keys["LST"], status, out)
    if key == "BTN_THUMBR":
        out = set_del_bit(keys["RST"], status, out)

    if key == "BTN_START":
        out = set_del_bit(keys["PLUS"], status, out)
    if key == "BTN_SELECT":
        out = set_del_bit(keys["MINUS"], status, out)

    return out


def print_in(buttons):
    for name, bm in keys.items():
        if buttons & bm:
            print(name, end=" ")
    print("")


if len(sys.argv) != 2:
    print("Usage: python3 input_pc.py SWITCH_IP")
    os._exit(1)

server_address = (sys.argv[1], 8080)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# -------- Main Program Loop -----------

keyout = 0
dx_l = 0
dy_l = 0
dx_r = 0
dy_r = 0

if len(devices.gamepads) == 0:
    print("No gamepads found")
    os._exit(1)

gamepad_type = str(devices.gamepads[0])
print(gamepad_type)
def input_poller():
    global keyout
    global dx_l
    global dy_l
    global dx_r
    global dy_r
    while(True):
        try:
            events = get_gamepad()
        except:
            print("Gamepad disconnected")
            os._exit(1)

        for event in events:
            if event.ev_type == "Key":
                keyout = check_keys(event.code, event.state, keyout)
            elif event.ev_type == "Absolute":
                if(event.code == "ABS_X"):
                    if gamepad_type == "Sony PLAYSTATION(R)3 Controller":
                        dx_l = (event.state-128) * 255
                    else:
                        dx_l = event.state
                    if(abs(dx_l) < 5000):
                        dx_l = 0
                if(event.code == "ABS_Y"):
                    if gamepad_type == "Sony PLAYSTATION(R)3 Controller":
                        dy_l = -(event.state-128) * 255
                    else:
                        dy_l = -event.state
                    if(abs(dy_l) < 5000):
                        dy_l = 0
                if(event.code == "ABS_RX"):
                    if gamepad_type == "Sony PLAYSTATION(R)3 Controller":
                        dx_r = (event.state-128) * 255
                    else:
                        dx_r = event.state
                    if(abs(dx_r) < 5000):
                        dx_r = 0
                if(event.code == "ABS_RY"):
                    if gamepad_type == "Sony PLAYSTATION(R)3 Controller":
                        dy_r = -(event.state-128) * 255
                    else:
                        dy_r = -event.state
                    if(abs(dy_r) < 5000):
                        dy_r = 0
                if gamepad_type != "Sony PLAYSTATION(R)3 Controller":
                    if(event.code == "ABS_Z"):
                        if event.state >= 500:
                            keyout = check_keys("BTN_TL2", 1, keyout)
                        else:
                            keyout = check_keys("BTN_TL2", 0, keyout)
                    if(event.code == "ABS_RZ"):
                        if event.state >= 500:
                            keyout = check_keys("BTN_TR2", 1, keyout)
                        else:
                            keyout = check_keys("BTN_TR2", 0, keyout)
                    if(event.code == "ABS_HAT0X"):
                        if event.state == 0:
                            keyout = check_keys("BTN_DPAD_LEFT", 0, keyout)
                            keyout = check_keys("BTN_DPAD_RIGHT", 0, keyout)
                        if event.state == 1:
                            keyout = check_keys("BTN_DPAD_RIGHT", 1, keyout)
                        if event.state == -1:
                            keyout = check_keys("BTN_DPAD_LEFT", 1, keyout)
                    if(event.code == "ABS_HAT0Y"):
                        if event.state == 0:
                            keyout = check_keys("BTN_DPAD_UP", 0, keyout)
                            keyout = check_keys("BTN_DPAD_DOWN", 0, keyout)
                        if event.state == 1:
                            keyout = check_keys("BTN_DPAD_DOWN", 1, keyout)
                        if event.state == -1:
                            keyout = check_keys("BTN_DPAD_UP", 1, keyout)


_thread.start_new_thread(input_poller, ())


while(True):
    #print(event.ev_type, event.code, event.state)
    sock.sendto(pack("<HQiiii", 0x3275, keyout, dx_l, dy_l, dx_r, dy_r), server_address)
    sleep(1/60)
