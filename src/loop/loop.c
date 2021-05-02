#include "loop/loop.h"

#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>

#include <stdlib.h>
#include <stddef.h>

#define LOOP_INITIAL_CAPACITY 16

struct fd_info
{
    int fd;
    loop_callback_fn * callback;
    void * context;
};

struct loop
{
    int write_fd;
    struct pollfd * fds;
    struct fd_info * fd_infos;
    size_t capacity;
    size_t size;
};

static void loop_on_interrupt(
    struct loop * loop,
    int id,
    int reason,
    void * context)
{
    (void) context;
    int fd = loop_get_fd_by_id(loop, id);

    switch (reason)
    {
        case LOOP_FD_READABLE:
            {
                char buffer;
                read(fd, &buffer, 1);
            }
            break;
        case LOOP_FD_CLEANUP:
            close(fd);
            break;
        default:
            break;
    }
}


struct loop *
loop_create(void)
{
    int sockets[2];
    int rc = socketpair(AF_LOCAL, SOCK_DGRAM, 0, sockets);
    if (0 != rc)
    {
        return NULL;
    }

    struct loop * loop = malloc(sizeof(struct loop));
    loop->fds = malloc(sizeof(struct pollfd) * LOOP_INITIAL_CAPACITY);
    loop->fd_infos = malloc(sizeof(struct fd_info) * LOOP_INITIAL_CAPACITY);
    loop->capacity = LOOP_INITIAL_CAPACITY;
    loop->size = 1;

    loop->write_fd = sockets[0];
    loop->fds[0].fd = sockets[1];
    loop->fds[0].events = POLLIN;
    loop->fds[0].revents = 0;
    loop->fd_infos[0].fd = sockets[1];
    loop->fd_infos[0].callback = loop_on_interrupt;
    loop->fd_infos[0].context = NULL;

    return loop;
}

void
loop_release(
    struct loop * loop)
{
    for (size_t id = 0; id < loop->size; id++)
    {
        loop_release_fd(loop, id);
    }

    close(loop->write_fd);
    free(loop->fd_infos);
    free(loop->fds);

    free(loop);
}

static void loop_callback(
        struct loop * loop,
        int id,
        int reason)
{
    int const fd = loop->fd_infos[id].fd;
    if (0 <= fd)
    {
        loop_callback_fn * callback = loop->fd_infos[id].callback;
        void * context = loop->fd_infos[id].context;

        callback(loop, id, reason, context);
    }
}

void
loop_service(
    struct loop * loop)
{
    int rc = poll(loop->fds, loop->size, -1);
    if (0 < rc)
    {
        for (size_t id = 0; id < loop->size; id++)
        {
            short int const revents = loop->fds[id].revents;

            if (0 != (POLLIN & revents))
            {
                loop_callback(loop, id, LOOP_FD_READABLE);
            }

            if (0 != (POLLOUT & revents))
            {
                loop_callback(loop, id, LOOP_FD_WRITABLE);                
            }

            if (0 != (POLLERR & revents))
            {
                loop_callback(loop, id, LOOP_FD_ERROR);                                
            }

            if (0 != (POLLHUP & revents))
            {
                loop_callback(loop, id, LOOP_FD_HUP);                                
            }
        }
    }
}

void
loop_interrupt(
    struct loop * loop)
{
    char buffer = 0x42;
    write(loop->write_fd, &buffer, 1);
}

int
loop_add_fd(
    struct loop * loop,
    int fd,
    loop_callback_fn * callback,
    void * context)
{
    int id = loop->size;

    // try to find free id    
    if (id >= loop->capacity)
    {
        for(int i = 0; i < loop->capacity; i++)
        {
            if (-1 == loop->fd_infos[i].fd)
            {
                id = i;
                break;
            }
        }
    }

    // increase space
    if (id >= loop->capacity)
    {
        loop->capacity *= 2;
        loop->fds = realloc(loop->fds, sizeof(struct pollfd) * loop->capacity);
        loop->fd_infos = realloc(loop->fd_infos, sizeof(struct fd_info) * loop->capacity);
    }

    loop->fds[id].fd = -1;
    loop->fds[id].events = 0;
    loop->fds[id].revents = 0;
    loop->fd_infos[id].fd = fd;
    loop->fd_infos[id].callback = callback;
    loop->fd_infos[id].context = context;

    if (id >= loop->size)
    {
        loop->size++;
    } 

    loop_callback(loop, id, LOOP_FD_INIT);

    return id;
}

void
loop_release_fd(
    struct loop * loop,
    int id)
{
    if ((0 <= id) && (id < loop->size) && (0 <= loop->fd_infos[id].fd))
    {
        loop_callback(loop, id, LOOP_FD_CLEANUP);

        loop->fds[id].fd = -1;
        loop->fds[id].events = 0;
        loop->fd_infos[id].fd = -1;
        loop->fd_infos[id].callback = NULL;
        loop->fd_infos[id].context = NULL;
    }
}

int
loop_get_id_of_fd(
    struct loop * loop,
    int fd)
{
    for (int id = 0; id < loop->size; id++)
    {
        if (loop->fd_infos[id].fd == fd)
        {
            return id;
        }
    }

    return -1;
}

int
loop_get_fd_by_id(
    struct loop * loop,
    int id)
{
    if ((0 <= id) && (id < loop->size))
    {
        return loop->fd_infos[id].fd;
    }

    return -1;
}

void
loop_begin_read(
    struct loop * loop,
    int id)
{
    if ((0 <= id) && (id < loop->size))
    {
        loop->fds[id].fd = loop->fd_infos[id].fd;
        loop->fds[id].events |= POLLIN;
    }
}

void
loop_end_read(
    struct loop * loop,
    int id)
{
    if ((0 <= id) && (id < loop->size))
    {
        loop->fds[id].events &= ~POLLIN;
        if (0 == loop->fds[id].events)
        {
            loop->fds[id].fd = -1;
        }
    }
}


void
loop_begin_write(
    struct loop * loop,
    int id)
{
    if ((0 <= id) && (id < loop->size))
    {
        loop->fds[id].fd = loop->fd_infos[id].fd;
        loop->fds[id].events |= POLLOUT;
    }
}

void
loop_end_write(
    struct loop * loop,
    int id)
{
    if ((0 <= id) && (id < loop->size))
    {
        loop->fds[id].events &= ~POLLOUT;
        if (0 == loop->fds[id].events)
        {
            loop->fds[id].fd = -1;
        }
    }
}
