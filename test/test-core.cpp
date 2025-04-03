#include "core.h"

static bool tsk_init_called = false;
uint tsk_loop0_count = 0;
uint tsk_loop1_count = 0;

static int tsk_init(void* w)
{
    w = w;
    jgb_function();
    jgb_assert(!tsk_init_called);
    tsk_init_called = true;
    return 0;
}

static int tsk_loop0(void* w)
{
    w = w;
    jgb_assert(tsk_init_called);
    ++ tsk_loop0_count;
    return 0;
}

static int tsk_loop1(void* w)
{
    w = w;
    jgb_assert(tsk_init_called);
    ++ tsk_loop1_count;
    return 0;
}

static void tsk_exit(void* w)
{
    w = w;
    jgb_assert(tsk_init_called);
    jgb_assert(tsk_loop0_count > 0);
    jgb_assert(tsk_loop1_count > 0);
    jgb_debug("{ tsk_loop0_count = %u , tsk_loop0_count = %u }",
              tsk_loop0_count, tsk_loop1_count);
}

static int test_core_init(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    int r;
    int ival;

    jgb_function();

    r = c->get("p0", ival);
    jgb_assert(!r);
    jgb_assert(ival == 250402);

    return 0;
}

static void test_core_release(void* conf)
{
    jgb_function();
    conf = conf;
}

static loop_ptr_t loop[] = { tsk_loop0, tsk_loop1, nullptr };

static jgb_task_t task
{
    .setup = tsk_init,
    .loop = loop,
    .exit = tsk_exit
};

jgb_api_t test_core
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test core",
    .init = test_core_init,
    .release = test_core_release,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .task = &task
};
