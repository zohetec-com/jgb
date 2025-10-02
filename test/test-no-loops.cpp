#include <jgb/core.h>
#include <jgb/helper.h>

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    w->set_user((void*)(intptr_t) 0x12345678);
    jgb_assert(w);
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb_assert((intptr_t) w->get_user() == 0x12345678);
    jgb_assert(w);
}

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = nullptr,
    .exit = tsk_exit
};

jgb_api_t no_loops_app =
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "no loops app",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
