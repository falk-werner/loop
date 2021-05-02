#ifndef LOOP_BUFFER_H
#define LOOP_BUFFER_H

#ifndef __cplusplus
#include <stddef.h>
#include <stdbool.h>
#else
#include <cstddef>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

extern ssize_t
loop_buffer_read(
    int fd,
    void * buffer,
    size_t * offset,
    size_t size);

extern ssize_t
loop_buffer_write(
    int fd,
    void * buffer,
    size_t * offset,
    size_t size);

#ifdef __cplusplus
}
#endif

#endif
