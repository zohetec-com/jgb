#include "core.h"
#include "helper.h"

struct context_7f2f5ef02b2f
{
    std::list<jgb::instance*> instances_;
};

static int create(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    jgb_assert(c);
    jgb::instance* inst = jgb::instance::get_instance(c);
    jgb_assert(inst);
    context_7f2f5ef02b2f* ctx = new context_7f2f5ef02b2f;
    inst->set_user(ctx);
    int r;
    jgb::value* val;
    r = c->get("subtask", &val);
    if(!r)
    {
        for(int i=0; i<val->len_; i++)
        {
            if(val->str_[i])
            {
                jgb::config* root_conf = jgb::core::get_instance()->root_conf();
                jgb::config* conf;
                r = root_conf->get(val->str_[i], &conf);
                if(!r)
                {
                    jgb::instance* sub_inst = jgb::instance::get_instance(conf);
                    if(sub_inst)
                    {
                        ctx->instances_.push_back(sub_inst);
                    }
                    else
                    {
                        jgb_fail("invalid. { subtask[%d] = %s }", i, val->str_[i]);
                    }
                }
                else
                {
                    jgb_fail("not found. { subtask[%d] = %s }", i, val->str_[i]);
                }
            }
        }

        bool run;
        r = c->get("auto", run);
        if(!r && run)
        {
            inst->start();
        }
    }

    return 0;
}

static void destroy(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    jgb_assert(c);
    jgb::instance* inst = jgb::instance::get_instance(c);
    jgb_assert(inst);
    inst->stop();
    context_7f2f5ef02b2f* ctx = static_cast<context_7f2f5ef02b2f*>(inst->get_user());
    delete ctx;
}

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb_assert(w);
    context_7f2f5ef02b2f* ctx = (context_7f2f5ef02b2f*) w->get_user();
    for(std::list<jgb::instance*>::iterator it = ctx->instances_.begin();
        it != ctx->instances_.end(); ++it)
    {
        jgb::instance* inst = *it;
        jgb_assert(inst);
        int r = inst->start();
        if(r)
        {
            std::string path;
            inst->conf_->get_path(path);
            jgb_fail("start instance failed. { instance = %s }", path.c_str());
        }
    }
    //jgb_assert(0);
    return 0;
}

static int tsk_loop(void*)
{
    jgb::sleep(1000);
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb_assert(w);
    context_7f2f5ef02b2f* ctx = (context_7f2f5ef02b2f*) w->get_user();
    for(std::list<jgb::instance*>::reverse_iterator it = ctx->instances_.rbegin();
        it != ctx->instances_.rend(); ++it)
    {
        jgb::instance* inst = *it;
        jgb_assert(inst);
        inst->stop();
    }
}

static loop_ptr_t loops[] = { tsk_loop, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t service
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "service",
    .init = nullptr,
    .release = nullptr,
    .create = create,
    .destroy = destroy,
    .commit = nullptr,
    .loop = &loop
};
