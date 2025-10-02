#include <jgb/core.h>
#include <jgb/helper.h>

struct context_d496dea5e006
{
    int no_used;
};

static int init(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    jgb_assert(c);
    jgb_assert(jgb::app::get_app(c));
    return 0;
}

static void release(void*)
{
    //jgb::config* c = (jgb::config*) conf;
}

static int create(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    jgb_assert(c);
    jgb_assert(jgb::instance::get_instance(c));
    return 0;
}

static void destroy(void*)
{
}

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    w->set_user(new context_d496dea5e006);
    jgb_assert(w);
    return 0;
}

static int tsk_loop(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_d496dea5e006* ctx = (context_d496dea5e006*) w->get_user();
    jgb_assert(w);
    jgb_assert(ctx);
    jgb::sleep(1000);
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_d496dea5e006* ctx = (context_d496dea5e006*) w->get_user();
    jgb_assert(w);
    delete ctx;
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
    .create = create,
    .destroy = destroy,
    .commit = nullptr,
    .loop = &loop
};
