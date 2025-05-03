#include <jgb/core.h>
#include <jgb/helper.h>

static int init(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    jgb_assert(c);
    return 0;
}

static void release(void*)
{
    //jgb::config* c = (jgb::config*) conf;
}

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb_assert(w);
    return 0;
}

static int tsk_loop(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb_assert(w);
    jgb::sleep(1000);
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb_assert(w);
}

static loop_ptr_t loops[] = { tsk_loop, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t template_app
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "template app",
    .init = init,
    .release = release,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
