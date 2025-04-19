#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include "log.h"
#include "app.h"
#include "helper.h"

static pid_t pid;
static int tsk_init(void*)
{
    pid = gettid();
    return 0;
}

static int tsk_loop(void*)
{
    pid_t pid_x = gettid();
    jgb_assert(pid == pid_x);
    jgb::sleep(100);
    return 0;
}

static void tsk_exit(void*)
{
    pid_t pid_x = gettid();
    jgb_assert(pid == pid_x);
}

static loop_ptr_t loops[] = { tsk_loop, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_single_worker
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test single worker",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
