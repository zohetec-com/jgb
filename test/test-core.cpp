#include "core.h"

static int tsk_init(void* w)
{
    jgb_debug("%p", w);
    return 0;
}

static int tsk_loop0(void* w)
{
    jgb_debug("%p", w);
    return 0;
}

static int tsk_loop1(void* w)
{
    jgb_debug("%p", w);
    return 0;
}

static void tsk_exit(void* w)
{
    jgb_debug("%p", w);
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

jgb_app_t test_core
{
    .version = MAKE_APP_VERSION(0, 1),
    .desc = "test core",
    .init = test_core_init,
    .release = test_core_release,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .task = &task
};
