#ifndef PROCESS_H
#define PROCESS_H

#include "ipc.h"

enum { PROCESS_CAPACITY = MAX_PROCESS_ID + 1 };

typedef struct {
    local_id id;
    int count;
    int reads[PROCESS_CAPACITY];
    int writes[PROCESS_CAPACITY];
    int events_fd;
} ProcessContext;

int write_all(int fd, const void *buffer, size_t size);

#endif
