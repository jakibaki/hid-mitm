#include "udp_input.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAXLINE 1024

static int sockfd = -1;

struct sockaddr_in servaddr, cliaddr;

void setup_socket()
{
    if (sockfd != -1)
    {
        close(sockfd);
    }
    
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 100000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
}

static u32 curIP = 0;
static int failed = 11;
static int counter = 0;
static struct input_msg cached_msg = {0};
u64 last_time;
int poll_udp_input(struct input_msg *buf)
{
    if (++counter != 3)
    {
        if (failed > 10)
            return -1;
        *buf = cached_msg;
        return 0;
    }
    counter = 0;

    // Wakeup detection
    u64 tmp_time = svcGetSystemTick();
    if (tmp_time - last_time > (19200000 / 10))
    {
        svcSleepThread(5e+8L);
        setup_socket();
        curIP = gethostid();
        tmp_time = svcGetSystemTick();
    }
    last_time = tmp_time;

    // Reduce the poll frequency to lower load.
    // If we're not "connected" right now we only check around every second
    if (failed > 10 && failed % 10 != 0)
    {
        failed++;
        return -1;
    }

    if (curIP != gethostid())
    {
        setup_socket();
        curIP = gethostid();
    }

    socklen_t len;
    int n;
    struct input_msg tmp_msg;
    n = recvfrom(sockfd, &tmp_msg, sizeof(struct input_msg),
                 MSG_WAITALL, (struct sockaddr *)&cliaddr,
                 &len);
    if (n <= 0 || tmp_msg.magic != INPUT_MSG_MAGIC)
    {
        failed++;
    }
    else
    {
        failed = 0;
        cached_msg = tmp_msg;
    }
    *buf = cached_msg;

    if (failed >= 10)
    {
        return -1;
    }

    return 0;
}