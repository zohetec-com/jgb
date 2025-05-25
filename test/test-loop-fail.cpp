#include <jgb/core.h>
#include <jgb/helper.h>

static int looped = 0;
static int exit_count = 0;
static void release(void*)
{
    jgb_assert(looped == 1);
    jgb_assert(exit_count == 1);
}

static int tsk_init(void*)
{
    return 0;
}

static int tsk_loop(void*)
{
    ++ looped;
    return JGB_ERR_FAIL;
}

static void tsk_exit(void*)
{
    ++ exit_count;
}

static loop_ptr_t loops[] = { tsk_loop, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_loop_fail
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "loop fail",
    .init = nullptr,
    .release = release,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
