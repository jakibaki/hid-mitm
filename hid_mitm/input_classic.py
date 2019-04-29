#!/usr/bin/env python3

from cwiid import *
import sys
import socket
import struct
import time
import threading

print("Press 1 and 2 on your Wiimote")
wm = Wiimote()
print("Connected")
time.sleep(1)

if wm.state["ext_type"] != EXT_CLASSIC:
    print("Classic controller not connected, exiting...")
    exit(wm)

wm.rpt_mode = RPT_CLASSIC

SWITCH_STICK_MIN = -32767
SWITCH_STICK_MAX = 32767

classic_to_switch_keys = {
    CLASSIC_BTN_A: 1,
    CLASSIC_BTN_B: 1 << 1,
    CLASSIC_BTN_X: 1 << 2,
    CLASSIC_BTN_Y: 1 << 3,
    CLASSIC_BTN_L: 1 << 6,
    CLASSIC_BTN_R: 1 << 7,
    CLASSIC_BTN_ZL: 1 << 8,
    CLASSIC_BTN_ZR: 1 << 9,
    CLASSIC_BTN_PLUS: 1 << 10,
    CLASSIC_BTN_MINUS: 1 << 11,
    CLASSIC_BTN_LEFT: 1 << 12,
    CLASSIC_BTN_UP: 1 << 13,
    CLASSIC_BTN_RIGHT: 1 << 14,
    CLASSIC_BTN_DOWN: 1 << 15
}

def classic_to_switch(classic):
    keyout = 0
    dx_l = 0
    dy_l = 0
    dx_r = 0
    dy_r = 0

    for key in classic_to_switch_keys:
        if classic["buttons"] & key:
            keyout |= classic_to_switch_keys[key]

    if classic["l_stick"][0] == 0: # Left
        keyout |= 1 << 16
        dx_l = SWITCH_STICK_MIN
    elif classic["l_stick"][1] == CLASSIC_L_STICK_MAX: # Up
        keyout |= 1 << 17
        dy_l = SWITCH_STICK_MAX
    elif classic["l_stick"][0] == CLASSIC_L_STICK_MAX: # Right
        keyout |= 1 << 18
        dx_l = SWITCH_STICK_MAX
    elif classic["l_stick"][1] == 0: # Down
        keyout |= 1 << 19
        dy_l = SWITCH_STICK_MIN

    if classic["r_stick"][0] == 0: # Left
        keyout |= 1 << 20
        dx_r = SWITCH_STICK_MIN
    elif classic["r_stick"][1] == CLASSIC_R_STICK_MAX: # Up
        keyout |= 1 << 21
        dy_r = SWITCH_STICK_MAX
    elif classic["r_stick"][0] == CLASSIC_R_STICK_MAX: # Right
        keyout |= 1 << 22
        dx_r = SWITCH_STICK_MAX
    elif classic["r_stick"][1] == 0: # Down
        keyout |= 1 << 23
        dy_r = SWITCH_STICK_MIN

    return (keyout, dx_l, dy_l, dx_r, dy_r)

keys = tuple([0] * 5)

if len(sys.argv) != 2:
    print("Usage: ./input_classic.py <SWITCH IP>")
    exit(wm)
serv_addr = (sys.argv[1], 8080)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def input_poller():
    global keys

    while True:
        keys = classic_to_switch(wm.state["classic"])

poller = threading.Thread(target=input_poller)
poller.start()

while True:
    try:
        sock.sendto(struct.pack("<HQiiii", 0x3275, keys[0], keys[1], keys[2], keys[3], keys[4]), serv_addr)
        time.sleep(1/60)
    except KeyboardInterrupt:
        exit(wm)