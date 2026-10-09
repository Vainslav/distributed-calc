#ifndef SUB_PROCESS_H
#define SUB_PROCESS_H

#include "process.h"

int log_event(ProcessContext *self, const char *text);
int await_phase(ProcessContext *self, MessageType type);
int sub_process_routine(ProcessContext *self);

#endif
