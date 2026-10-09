#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "sub_process.h"

typedef int PipeMatrix[PROCESS_CAPACITY][PROCESS_CAPACITY][2];

static void close_matrix(PipeMatrix pipes)
{
    int i, j, end;
    for (i = 0; i < PROCESS_CAPACITY; ++i)
        for (j = 0; j < PROCESS_CAPACITY; ++j)
            for (end = 0; end < 2; ++end)
                if (pipes[i][j][end] >= 0) {
                    close(pipes[i][j][end]);
                    pipes[i][j][end] = -1;
                }
}

static void select_channels(ProcessContext *ctx, PipeMatrix pipes)
{
    int i, j;
    for (i = 0; i < ctx->count; ++i) {
        for (j = 0; j < ctx->count; ++j) {
            if (i == j) continue;
            if (j == ctx->id) {
                ctx->reads[i] = pipes[i][j][0];
                pipes[i][j][0] = -1;
            }
            if (i == ctx->id) {
                ctx->writes[j] = pipes[i][j][1];
                pipes[i][j][1] = -1;
            }
        }
    }
    close_matrix(pipes);
}

static void close_context(ProcessContext *ctx)
{
    int peer;
    for (peer = 0; peer < ctx->count; ++peer) {
        if (ctx->reads[peer] >= 0) close(ctx->reads[peer]);
        if (ctx->writes[peer] >= 0) close(ctx->writes[peer]);
    }
    if (ctx->events_fd >= 0) close(ctx->events_fd);
}

int main(int argc, char *argv[])
{
    ProcessContext ctx;
    PipeMatrix pipes;
    pid_t children[PROCESS_CAPACITY];
    char *end;
    long child_count;
    int i, j, k, pipes_fd, created = 0, result = 0;

    if (argc != 3 || strcmp(argv[1], "-p") != 0) return EXIT_FAILURE;
    errno = 0;
    child_count = strtol(argv[2], &end, 10);
    if (errno || end == argv[2] || *end || child_count < 1 || child_count > 10)
        return EXIT_FAILURE;
    ctx.id = PARENT_ID;
    ctx.count = (int)child_count + 1;
    ctx.events_fd = -1;
    for (i = 0; i < PROCESS_CAPACITY; ++i) {
        ctx.reads[i] = ctx.writes[i] = -1;
        for (j = 0; j < PROCESS_CAPACITY; ++j)
            for (k = 0; k < 2; ++k) pipes[i][j][k] = -1;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) return EXIT_FAILURE;
    ctx.events_fd = open(events_log, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
    if (ctx.events_fd < 0) return EXIT_FAILURE;
    pipes_fd = open(pipes_log, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (pipes_fd < 0) {
        close_context(&ctx);
        return EXIT_FAILURE;
    }
    for (i = 0; i < ctx.count && !result; ++i) {
        for (j = 0; j < ctx.count; ++j) {
            char line[128];
            int len;
            if (i == j) continue;
            if (pipe(pipes[i][j]) < 0) { result = 1; break; }
            len = snprintf(line, sizeof(line),
                           "Pipe %d -> %d: read=%d write=%d\n", i, j,
                           pipes[i][j][0], pipes[i][j][1]);
            if (len < 0 || (size_t)len >= sizeof(line) ||
                write_all(pipes_fd, line, (size_t)len)) { result = 1; break; }
        }
    }
    close(pipes_fd);
    for (i = 1; i < ctx.count && !result; ++i) {
        pid_t pid = fork();
        if (pid < 0) { result = 1; break; }
        if (pid == 0) {
            int child_result;
            ctx.id = (local_id)i;
            select_channels(&ctx, pipes);
            child_result = sub_process_routine(&ctx);
            close_context(&ctx);
            _exit(child_result ? EXIT_FAILURE : EXIT_SUCCESS);
        }
        children[created++] = pid;
    }
    if (!result) {
        select_channels(&ctx, pipes);
        result = await_phase(&ctx, STARTED) || await_phase(&ctx, DONE);
    } else {
        close_matrix(pipes);
    }
    if (result)
        for (i = 0; i < created; ++i) kill(children[i], SIGTERM);
    close_context(&ctx);
    for (i = 0; i < created; ++i) {
        int status;
        pid_t pid;
        do { pid = wait(&status); } while (pid < 0 && errno == EINTR);
        if (pid < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
            result = 1;
    }
    return result ? EXIT_FAILURE : EXIT_SUCCESS;
}
