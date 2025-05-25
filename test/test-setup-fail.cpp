#include <jgb/core.h>
#include <jgb/helper.h>

static int looped = 0;
static void release(void*)
{
    jgb_assert(!looped);
}

static int tsk_init(void*)
{
    return JGB_ERR_FAIL;
}

static int tsk_loop(void*)
{
    ++ looped;
    return 0;
}

static void tsk_exit(void*)
{
    jgb_assert(0);
}

static loop_ptr_t loops[] = { tsk_loop, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_setup_fail
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "setup fail",
    .init = nullptr,
    .release = release,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
