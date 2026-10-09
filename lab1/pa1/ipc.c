#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "process.h"

int write_all(int fd, const void *buffer, size_t size)
{
    const char *next = buffer;
    while (size) {
        ssize_t n = write(fd, next, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        next += n;
        size -= (size_t)n;
    }
    return 0;
}

static int read_all(int fd, void *buffer, size_t size)
{
    char *next = buffer;
    while (size) {
        ssize_t n = read(fd, next, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        next += n;
        size -= (size_t)n;
    }
    return 0;
}

static int valid_peer(const ProcessContext *self, local_id peer)
{
    return self && self->count > 0 && self->count <= PROCESS_CAPACITY &&
           peer >= 0 && peer < self->count && peer != self->id;
}

static int receive_rest(int fd, Message *msg, size_t header_read)
{
    if (read_all(fd, (char *)&msg->s_header + header_read,
                 sizeof(MessageHeader) - header_read)) return -1;
    if (msg->s_header.s_magic != MESSAGE_MAGIC ||
        msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    return read_all(fd, msg->s_payload, msg->s_header.s_payload_len);
}

int send(void *self, local_id dst, const Message *msg)
{
    ProcessContext *ctx = self;
    if (!valid_peer(ctx, dst) || !msg || ctx->writes[dst] < 0 ||
        msg->s_header.s_magic != MESSAGE_MAGIC ||
        msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    return write_all(ctx->writes[dst], msg,
                     sizeof(MessageHeader) + msg->s_header.s_payload_len);
}

int send_multicast(void *self, const Message *msg)
{
    ProcessContext *ctx = self;
    int peer;
    if (!ctx || !msg || ctx->count < 1 || ctx->count > PROCESS_CAPACITY)
        return -1;
    for (peer = 0; peer < ctx->count; ++peer) {
        if (peer != ctx->id && send(ctx, (local_id)peer, msg)) return -1;
    }
    return 0;
}

int receive(void *self, local_id from, Message *msg)
{
    ProcessContext *ctx = self;
    if (!valid_peer(ctx, from) || !msg || ctx->reads[from] < 0) return -1;
    return receive_rest(ctx->reads[from], msg, 0);
}

int receive_any(void *self, Message *msg)
{
    ProcessContext *ctx = self;
    const struct timespec pause = {0, 1000000};
    if (!ctx || !msg || ctx->count < 1 || ctx->count > PROCESS_CAPACITY)
        return -1;
    for (;;) {
        int peer, active = 0;
        for (peer = 0; peer < ctx->count; ++peer) {
            int fd, flags, saved_errno;
            ssize_t n;
            if (peer == ctx->id || ctx->reads[peer] < 0) continue;
            fd = ctx->reads[peer];
            flags = fcntl(fd, F_GETFL);
            if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
                return -1;
            do {
                n = read(fd, &msg->s_header, 1);
            } while (n < 0 && errno == EINTR);
            saved_errno = errno;
            if (fcntl(fd, F_SETFL, flags) < 0) return -1;
            if (n > 0) return receive_rest(fd, msg, 1);
            if (n == 0) {
                close(fd);
                ctx->reads[peer] = -1;
            } else if (saved_errno != EAGAIN && saved_errno != EWOULDBLOCK) {
                return -1;
            } else {
                active = 1;
            }
        }
        if (!active) return -1;
        nanosleep(&pause, NULL);
    }
}
