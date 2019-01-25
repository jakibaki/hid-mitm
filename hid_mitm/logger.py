from struct import pack, unpack
import socket
from keyboard import is_pressed
from time import sleep
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

def check_keys():
    out = 0
    if is_pressed('a'):
        out |= keys["Y"]
    if is_pressed('s'):
        out |= keys["X"]
    if is_pressed('z'):
        out |= keys["B"]
    if is_pressed('x'):
        out |= keys["A"]
    if is_pressed('up'):
        out |= keys["DU"]
    if is_pressed('down'):
        out |= keys["DD"]
    if is_pressed('left'):
        out |= keys["DL"]
    if is_pressed('right'):
        out |= keys["DR"]
    return out

def print_in(buttons):
    for name, bm in keys.items():
        if buttons & bm:
            print(name, end=" ")
    print("")


# create an INET, STREAMing socket
serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# bind the socket to a public host, and a well-known port
serversocket.bind((socket.gethostname(), 5555))
# become a server socket
serversocket.listen(1)
(clientsock, address) = serversocket.accept()
print("Got connection!")
"""
while True:
    buttons, = unpack("<Q", clientsock.recv(8))
    #print(format(buttons, '#064b'))
    print_in(buttons)
"""

while True:
    line = input().split(" ")
    out = 0
    #for name in line:
    #    try:
    #        out |= keys[name]
    #    except KeyError:
    #        pass
    out = check_keys()
    print_in(out)
    clientsock.send(pack("<Q", out))
    sleep(0.05)