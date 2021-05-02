#include <loop/loop.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#define SERVER_PORT 54321
#define BACKLOG 5

struct server_context
{
    int fd;
};

struct client_context
{
    int fd;
    char buffer[10];
    size_t buffer_size;
};

static bool shutdown_requested = false;

static void server_create(
    struct server_context * ctx)
{
    int server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (0 > server_socket)
    {
        fprintf(stderr, "error: failed to create server socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(SERVER_PORT);
    int rc = bind(server_socket, (struct sockaddr*) &address, sizeof(address));
    if (0 != rc)
    {
        close(server_socket);
        fprintf(stderr, "error: failed to bind server socket\n");
        exit(EXIT_FAILURE);
    }


    rc = listen(server_socket, BACKLOG);
    if (0 != rc)
    {
        close(server_socket);
        fprintf(stderr, "error: failed to enter listen mode\n");
        exit(EXIT_FAILURE);
    }

    ctx->fd = server_socket;
}

static void
client_callback(
    struct loop * loop,
    int id,
    int reason,
    void * context
)
{
    struct client_context * ctx = context;

    switch (reason)
    {
        case LOOP_FD_INIT:
            {
                loop_begin_read(loop, id);
            }
            break;
        case LOOP_FD_READABLE:
            {
                ssize_t len = read(ctx->fd, ctx->buffer, 9);
                if (0 < len)
                {
                    ctx->buffer[len] = '\0';
                    ctx->buffer_size = len;
                    loop_end_read(loop, id);
                    loop_begin_write(loop, id);
                    puts(ctx->buffer);
                }
                else
                {
                    loop_release_fd(loop, id);
                }
            }
            break;
        case LOOP_FD_WRITABLE:
            write(ctx->fd, ctx->buffer, ctx->buffer_size);
            loop_end_write(loop, id);
            loop_release_fd(loop, id);
            break;
        case LOOP_FD_ERROR:
            break;
        case LOOP_FD_CLEANUP:
            close(ctx->fd);
            free(ctx);
            break;
        default:
            break;
    }
}

static void
server_callback(
    struct loop * loop,
    int id,
    int reason,
    void * context
)
{
    struct server_context * ctx = context;

    switch (reason)
    {
        case LOOP_FD_INIT:
            {
                loop_begin_read(loop, id);
            }
            break;
        case LOOP_FD_READABLE:
            {
                int client_fd = accept(ctx->fd, NULL, NULL);
                if (0 <= client_fd)
                {
                    struct client_context * client = malloc(sizeof(struct client_context));
                    client->fd = client_fd;
                    loop_add_fd(loop, client_fd, &client_callback, client);
                }
            }
            break;
        case LOOP_FD_HUP: // fall-through
        case LOOP_FD_ERROR:
            puts("error");
            shutdown_requested = true;
            break;
        case LOOP_FD_CLEANUP:
            close(ctx->fd);
            break;
        default:
            break;
    }
}

void on_interrupt(int signal_number)
{
    (void) signal_number;
    shutdown_requested = true;
}

int main(int argc, char * argv[])
{
    signal(SIGINT, &on_interrupt);
    struct loop * loop = loop_create();

    struct server_context ctx;
    server_create(&ctx);
    loop_add_fd(loop, ctx.fd, &server_callback, &ctx);

    while (!shutdown_requested)
    {
        loop_service(loop);
    }

    loop_release(loop);
    return EXIT_SUCCESS;
}