#ifndef LOOP_H
#define LOOP_H

#ifdef __cplusplus
extern "C"
{
#endif

#define LOOP_FD_INIT     101
#define LOOP_FD_CLEANUP  102
#define LOOP_FD_READABLE 103
#define LOOP_FD_WRITABLE 104
#define LOOP_FD_ERROR    105
#define LOOP_FD_HUP      106

#define LOOP_USER 10000

struct loop;

typedef void
loop_callback_fn(
    struct loop * loop,
    int id,
    int reason,
    void * context);

extern struct loop *
loop_create(void);

extern void
loop_release(
    struct loop * loop);

extern void
loop_service(
    struct loop * loop);

extern void
loop_interrupt(
    struct loop * loop);

extern int
loop_add_fd(
    struct loop * loop,
    int fd,
    loop_callback_fn * callback,
    void * context);

extern void
loop_release_fd(
    struct loop * loop,
    int id);

extern int
loop_get_id_of_fd(
    struct loop * loop,
    int fd);

extern int
loop_get_fd_by_id(
    struct loop * loop,
    int id);

extern void
loop_begin_read(
    struct loop * loop,
    int id);

extern void
loop_end_read(
    struct loop * loop,
    int id);

extern void
loop_begin_write(
    struct loop * loop,
    int id);

extern void
loop_end_write(
    struct loop * loop,
    int id);

#ifdef __cplusplus
}
#endif

#endif
