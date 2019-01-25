"""
 Pygame base template for opening a window
 
 Sample Python/Pygame Programs
 Simpson College Computer Science
 http://programarcadegames.com/
 http://simpson.edu/computer-science/
 
 Explanation video: http://youtu.be/vRB_983kUMc
"""

import pygame
from struct import pack, unpack
import socket
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


def check_keys(pressed):
    out = 0
    if pressed[pygame.K_a]:
        out |= keys["Y"]
    if pressed[pygame.K_s]:
        out |= keys["X"]
        
    if pressed[pygame.K_z]:
        out |= keys["B"]
        
    if pressed[pygame.K_x]:
        out |= keys["A"]

    if pressed[pygame.K_LEFT]:
        out |= keys["DL"]

    if pressed[pygame.K_DOWN]:
        out |= keys["DD"]

    if pressed[pygame.K_UP]:
        out |= keys["DU"]

    if pressed[pygame.K_RIGHT]:
        out |= keys["DR"]

    return out


def print_in(buttons):
    for name, bm in keys.items():
        if buttons & bm:
            print(name, end=" ")
    print("")


pygame.init()

# Set the width and height of the screen [width, height]
size = (100, 100)
screen = pygame.display.set_mode(size)

pygame.display.set_caption("My Game")

# Loop until the user clicks the close button.
done = False

# Used to manage how fast the screen updates
clock = pygame.time.Clock()


# create an INET, STREAMing socket
serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# bind the socket to a public host, and a well-known port
serversocket.bind((socket.gethostname(), 5555))
# become a server socket
serversocket.listen(1)
(clientsock, address) = serversocket.accept()
print("Got connection!")

# -------- Main Program Loop -----------
while not done:
    # --- Main event loop
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            done = True
    pressed = pygame.key.get_pressed()
    out = check_keys(pressed)
    clientsock.send(pack("<Q", out))
    # --- Game logic should go here

    # --- Screen-clearing code goes here

    # Here, we clear the screen to white. Don't put other drawing commands
    # above this, or they will be erased with this command.
    # --- Go ahead and update the screen with what we've drawn.
    pygame.display.flip()
    # --- Limit to 30 frames per second
    clock.tick(30)


# Close the window and quit.
pygame.quit()
