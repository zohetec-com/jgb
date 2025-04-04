#include "core.h"

static bool tsk_init_called = false;
uint tsk_loop0_count = 0;
uint tsk_loop1_count = 0;
static bool tsk_exit_called = false;

static int tsk_init(void* w)
{
    w = w;
    jgb_function();
    jgb_assert(!tsk_init_called);
    jgb_assert(!tsk_loop0_count);
    jgb_assert(!tsk_loop1_count);
    jgb_assert(!tsk_exit_called);
    tsk_init_called = true;
    return 0;
}

static int tsk_loop0(void* w)
{
    w = w;
    jgb_assert(tsk_init_called);
    jgb_assert(!tsk_exit_called);
    ++ tsk_loop0_count;
    return 0;
}

static int tsk_loop1(void* w)
{
    w = w;
    jgb_assert(tsk_init_called);
    jgb_assert(!tsk_exit_called);
    ++ tsk_loop1_count;
    return 0;
}

static void tsk_exit(void* w)
{
    w = w;
    jgb_assert(tsk_init_called);
    jgb_assert(!tsk_exit_called);
    jgb_assert(tsk_loop0_count);
    jgb_assert(tsk_loop1_count);
    jgb_debug("{ tsk_loop0_count = %u , tsk_loop0_count = %u }",
              tsk_loop0_count, tsk_loop1_count);
    tsk_exit_called = true;
}

static int test_core_init(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    int r;
    int ival;

    jgb_function();

    jgb_assert(!tsk_init_called);
    jgb_assert(!tsk_loop0_count);
    jgb_assert(!tsk_loop1_count);
    jgb_assert(!tsk_exit_called);

    r = c->get("p0", ival);
    jgb_assert(!r);
    jgb_assert(ival == 250402);

    return 0;
}

static void test_core_release(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    int r;
    int ival;

    jgb_function();

    jgb_assert(tsk_init_called);
    jgb_assert(tsk_exit_called);
    jgb_assert(tsk_loop0_count);
    jgb_assert(tsk_loop1_count);

    r = c->get("p0", ival);
    jgb_assert(!r);
    jgb_assert(ival == 250402);

    // reset
    tsk_init_called = false;
    tsk_loop0_count = 0;
    tsk_loop1_count = 0;
    tsk_exit_called = false;
}

static loop_ptr_t loops[] = { tsk_loop0, tsk_loop1, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
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
    .loop = &loop
};
