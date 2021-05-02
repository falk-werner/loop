#include <loop/loop.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 54321

enum state
{
    CLIENT_STATE_INIT,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_CONNECTED,
    CLIENT_STATE_SHUTDOWN
};

struct context
{
    enum state state;
    int fd;
};

static void
client_callback(
    struct loop * loop,
    int id,
    int reason,
    void * context
)
{
    struct context * ctx = context;

    switch (reason)
    {
        case LOOP_FD_INIT:
            {
                struct sockaddr_in address;
                memset(&address, 0, sizeof(address));
                address.sin_family = AF_INET;
                address.sin_port = htons(SERVER_PORT);
                address.sin_addr.s_addr = htonl(INADDR_ANY);

                int rc = connect(ctx->fd, (struct sockaddr*) &address, sizeof(address));
                if (0 == rc)
                {
                    loop_begin_write(loop, id);
                    ctx->state = CLIENT_STATE_CONNECTED;
                }
                else
                {
                    if (errno == EINPROGRESS)
                    {
                        loop_begin_write(loop, id);
                        ctx->state = CLIENT_STATE_CONNECTING;
                    }
                    else
                    {
                        fprintf(stderr, "error: failed to connect\n");
                        ctx->state = CLIENT_STATE_SHUTDOWN;
                    }
                }
            }
            break;
        case LOOP_FD_READABLE:
            {
                char buffer[10];
                ssize_t len = read(ctx->fd, buffer, 9);
                if (0 < len)
                {
                    buffer[len] = '\0';
                    puts(buffer);
                }
                loop_end_read(loop, id);
                ctx->state = CLIENT_STATE_SHUTDOWN;
            }
            break;
        case LOOP_FD_WRITABLE:
            {
                if (ctx->state == CLIENT_STATE_CONNECTING)
                {
                    ctx->state = CLIENT_STATE_CONNECTED;
                }

                char buffer[] = "Hello";
                write(ctx->fd, buffer, strlen(buffer));
                loop_end_write(loop, id);
                loop_begin_read(loop, id);
            }
            break;
        case LOOP_FD_ERROR:
            ctx->state = CLIENT_STATE_SHUTDOWN;
            break;
        case LOOP_FD_HUP:
            ctx->state = CLIENT_STATE_SHUTDOWN;
            break;
        case LOOP_FD_CLEANUP:
            close(ctx->fd);
            ctx->state = CLIENT_STATE_SHUTDOWN;
            break;
        default:
            break;
    }
}

int main(int argc, char * argv[])
{
    struct loop * loop = loop_create();

    struct context ctx;
    ctx.fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (0 > ctx.fd)
    {
        fprintf(stderr, "error: failed to create server socket\n");
        exit(EXIT_FAILURE);
    }

    loop_add_fd(loop, ctx.fd, &client_callback, &ctx);

    while (ctx.state != CLIENT_STATE_SHUTDOWN)
    {
        loop_service(loop);
    }

    loop_release(loop);
    return EXIT_SUCCESS;    
}