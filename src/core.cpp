#include "config_factory.h"
#include "core.h"
#include "error.h"
#include "helper.h"
#include <string>

namespace jgb
{
worker::worker(int id, task* task)
    : id_(id),
      task_(task),
      normal_(true),
      thread_(nullptr)
{
}

task::task(struct app* app)
    : app_(app),
      run_(false),
      state_(task_state_idle)
{
    worker_.resize(0);
    if(app
            && app->api_
            && app->api_->task)
    {
        jgb_task_t* task = app->api_->task;
        for(int i=0;;i++)
        {
            if(task->loop[i])
            {
                jgb_debug("add worker. { app.name = %s, id = %d }", app->name_.c_str(), i);
                worker_.push_back(worker(i, this));
            }
            else
            {
                break;
            }
        }
    }
}

int task::start()
{
    return 0;
}

int task::stop()
{
    return 0;
}

app::app(const char* name, jgb_api_t* api, config* conf)
    : name_(name),
      api_(api),
      conf_(conf)
{
}

core::core()
{
    conf_dir_ = "/etc/jgb";
    app_conf_ = new config;
}

core::~core()
{
    delete app_conf_;
}

core* core::get_instance()
{
    static core a;
    return &a;
}

int core::set_conf_dir(const char* dir)
{
    if(!dir)
    {
        return JGB_ERR_INVALID;
    }

    // TODO: 检查目录是否存在？
    conf_dir_ = dir;
    jgb_notice("jgb setting changed: { conf_dir_ = %s }", conf_dir_);

    return 0;
}

int core::install(const char* name, jgb_api_t* api)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    struct app* papp = find(name);
    if(papp)
    {
        jgb_warning("app already exist. { name = %s, desc = %s }", name, papp->api_->desc);
        return JGB_ERR_IGNORED;
    }

    std::string conf_file_path = std::string(conf_dir_) + '/' + name + ".json";
    config* conf = config_factory::create(conf_file_path.c_str());
    app_conf_->add(name, conf);
    //jgb_debug("{ name = %s, conf = %p }", name, conf);

    app_.push_back(app(name, api, conf));
    struct app& app = app_.back();

    if(api)
    {
        if(api->version != current_api_interface_version)
        {
            jgb_fail("invalid api version. { api.version = %x, required = %x }",
                     api->version, current_api_interface_version);
            return JGB_ERR_NOT_SUPPORT;
        }

        if(api->init)
        {
            int r = api->init(conf);
            if(!r)
            {
                jgb_ok("install %s. { desc = \"%s\" }", name, api->desc);
            }
            else
            {
                jgb_fail("install %s. { init() = %d }", name, r);
                return JGB_ERR_FAIL;
            }
        }

        app.task_ = task(&app);
    }

    return 0;
}

struct app* core::find(const char* name)
{
    for(auto it = app_.begin(); it != app_.end(); ++it)
    {
        if(!strcmp(name, (*it).name_.c_str()))
        {
            return &(*it);
        }
    }
    return nullptr;
}

struct core_worker
{
    void operator()(struct worker* w)
    {
        jgb_function();

        if(!w)
        {
            jgb_bug();
            return;
        }
    }
};

int core::start(const char* path)
{
    std::string base;
    int r;
    const char* s = path;
    const char* e;

    jgb_fail("start task. { path = \"%s\" }", path);

    r = jgb::jpath_parse(&s, &e);
    jgb_debug("{ r = %d }", r);
    if(!r)
    {
        std::string name(s, e);
        struct app* app;
        app = find(name.c_str());
        jgb_debug("{ app = %p }", app);
        if(app)
        {
            return app->task_.start();
        }
    }

    return JGB_ERR_INVALID;
}

int core::stop(const char* path)
{
    std::string base;
    int r;
    const char* s = path;
    const char* e;

    r = jgb::jpath_parse(&s, &e);
    if(!r)
    {
        std::string name(s, e);
        struct app* app;
        app = find(name.c_str());
        if(app)
        {
            return app->task_.stop();
        }
    }
    jgb_fail("start task. { path = \"%s\" }", path);
    return JGB_ERR_INVALID;
}

}
