#include "loop/buffer.h"

#include <unistd.h>>

ssize_t
loop_buffer_read(
    int fd,
    void * buffer,
    size_t * offset,
    size_t size)
{
    char * buf = buffer;
    size_t remaining = size - *offset;
    ssize_t len = read(fd, &buf[*offset], remaining);
    if (0 < len)
    {
        *offset = (*offset) + len;
        return *offset;
    }

    return len;
}

ssize_t
loop_buffer_write(
    int fd,
    void * buffer,
    size_t * offset,
    size_t size)
{
    char * buf = buffer;
    size_t remaining = size - *offset;
    ssize_t len = write(fd, &buf[*offset], remaining);
    if (0 < len)
    {
        *offset = (*offset) + len;
        return *offset;
    }

    return len;
}
