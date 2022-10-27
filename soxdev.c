
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define bool char
#define true 1
#define false 0

#define ERR_BAD_FD          100
#define ERR_BAD_BIND        101
#define ERR_BAD_ADDR_CONV   102
#define ERR_BAD_PORT_CONV   103

struct sock
{
    bool is_tcp;
    bool is_server;
    int fd;
    char ip[16];
    int port;
};

static volatile bool active = true;

int setup_sock(struct sock* s);
int close_sock(struct sock* s);
int check_sock(struct sock* s);
void sigint() { active = false; }

int main(int argc, char* argv[])
{
    printf("Hello soxdev.\n");

    signal(SIGINT, sigint);

    struct sock s;
    s.is_server = true;
    s.is_tcp = true;
    strcpy(s.ip, "127.0.0.1");
    s.port = 50501;

    int retval = setup_sock(&s);

    if (retval != 0)
    {
        printf("ERR: Code %d.\n", retval);
        return retval;
    }

    while (active)
    {
        check_sock(&s);
        sleep(1);
    }

    printf("\nINF: Success. Closing.\n");

    close_sock(&s);

    return 0;
}

int setup_sock(struct sock* s)
{
    if (s->port <= 1024 || s->port >= 65534)
    {
        return ERR_BAD_PORT_CONV;
    }

    if (s->is_tcp == true)
    {
        s->fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    else
    {
        s->fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (s->fd == -1)
    {
        return ERR_BAD_FD;
    }

    struct sockaddr_in bindaddr;
    memset(&bindaddr, 0, sizeof(struct sockaddr_in));

    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = inet_addr(s->ip);
    bindaddr.sin_port = htons(s->port);

    if (bindaddr.sin_addr.s_addr == ((in_addr_t)(-1)))
    {
        return ERR_BAD_ADDR_CONV;
    }

    int retval = bind(s->fd, (struct sockaddr*) &bindaddr, sizeof(bindaddr));

    if (retval == -1)
    {
        return ERR_BAD_BIND;
    }

    return 0;
}

int close_sock(struct sock* s)
{
    close(s->fd);
}

int check_sock(struct sock* s)
{
    printf(".");
    fflush(stdout);
    return 0;
}
