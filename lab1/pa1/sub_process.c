#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sub_process.h"
#include "pa1.h"

int log_event(ProcessContext *self, const char *text)
{
    size_t len = strlen(text);
    return write_all(self->events_fd, text, len) ||
           write_all(STDOUT_FILENO, text, len);
}

static int announce(ProcessContext *self, MessageType type)
{
    Message msg;
    int len;
    memset(&msg, 0, sizeof(msg));
    if (type == STARTED) {
        len = snprintf(msg.s_payload, sizeof(msg.s_payload), log_started_fmt,
                       self->id, (int)getpid(), (int)getppid());
    } else {
        len = snprintf(msg.s_payload, sizeof(msg.s_payload), log_done_fmt,
                       self->id);
    }
    if (len < 0 || (size_t)len >= sizeof(msg.s_payload)) return -1;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = (uint16_t)len;
    msg.s_header.s_type = type;
    msg.s_header.s_local_time = 0;
    if (log_event(self, msg.s_payload)) return -1;
    return send_multicast(self, &msg);
}

int await_phase(ProcessContext *self, MessageType type)
{
    int peer;
    char line[128];
    Message msg;
    for (peer = 1; peer < self->count; ++peer) {
        if (peer == self->id) continue;
        if (receive(self, (local_id)peer, &msg) ||
            msg.s_header.s_type != type) return -1;
    }
    snprintf(line, sizeof(line), type == STARTED ?
             log_received_all_started_fmt : log_received_all_done_fmt, self->id);
    return log_event(self, line);
}

int sub_process_routine(ProcessContext *self)
{
    return announce(self, STARTED) || await_phase(self, STARTED) ||
           announce(self, DONE) || await_phase(self, DONE);
}
