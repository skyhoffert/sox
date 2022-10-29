
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#define bool char
#define true 1
#define false 0

#define ERR_BAD_FD          100
#define ERR_BAD_BIND        101
#define ERR_BAD_ADDR_CONV   102
#define ERR_BAD_PORT_CONV   103
#define ERR_BAD_LISTEN      104

#define SOX_NONE            100
#define SOX_CONNECT         101
#define SOX_RECV            102

struct sock
{
    bool is_tcp;
    bool is_server;
    int fd;
    char ip[16];
    int port;

    int connfd; // Only for TCP sockets.
};

static volatile bool active = true;

int setup_sock(struct sock* s);
int close_sock(struct sock* s);
int tick_sock(struct sock* s, int* stype, char* buf);

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

    int stype = 0;
    char buf[1024];

    while (active)
    {
        retval = tick_sock(&s, &stype, buf);

        if (retval != 0)
        {
            printf("ERR: Tick retval %d.\n", retval);
            break;
        }

        sleep(1);
    }

    close_sock(&s);

    printf("\nINF: Exiting.\n");

    return 0;
}

int setup_sock(struct sock* s)
{
    s->connfd = -1;

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

    if (s->is_tcp == true)
    {
        retval = listen(s->fd, 1);
        if (retval != 0)
        {
            return ERR_BAD_LISTEN;
        }
    }

    return 0;
}

int close_sock(struct sock* s)
{
    close(s->fd);
}

int tick_sock(struct sock* s, int* stype, char* buf)
{
    int retval = 0;
    struct timeval tv;

    if (s->connfd == -1)
    {
        if (s->is_server == true)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(s->fd, &fds);
            tv.tv_sec = 0;
            tv.tv_usec = 100000;

            retval = select(s->fd+1, &fds, NULL, NULL, &tv);

            if (retval == -1)
            {
                printf("ERR: select error in tcp, server, listen.\n");
            }
            else if (retval == 0)
            {
                printf(".");
                fflush(stdout);
            }
            else if (FD_ISSET(s->fd, &fds))
            {
                s->connfd = accept(s->fd, NULL, 0);
                if (s->connfd == -1)
                {
                    printf("ERR: On tcp, server, accept.\n");
                }
                else
                {
                    printf("INF: tcp, server, accept: connect!\n");
                }
            }
        }
    }
    else
    {
        char b[1];
        retval = recv(s->connfd, b, 1, MSG_PEEK | MSG_DONTWAIT);

        if (retval > 0)
        {
            retval = recv(s->connfd, buf, 1024, 0);
            printf("data: %s\n", buf);
        }
        else if (retval == -1)
        {
            printf("%dx", retval);
            fflush(stdout);
        }
        else if (retval == 0)
        {
            printf("ERR: Conn dropped.\n");
            close(s->connfd);
            s->connfd = -1;
        }
    }

    return 0;
}
