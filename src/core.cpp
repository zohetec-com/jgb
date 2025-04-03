#include "config_factory.h"
#include "core.h"
#include "error.h"
#include "helper.h"
#include <string>

namespace jgb
{
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

int core::install(const char* name, jgb_app_t* api)
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

    // {} ???
    //struct app app_x { name, api, conf, {} };
    //app_.push_back(app_x);
    app_.push_back({ name, api, conf, {} });
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

        if(api->task && api->task->loop)
        {
            for(int i=0;;i++)
            {
                if((!i && api->task->setup) || api->task->loop[i])
                {
                    struct worker w {i, &app_.back(), true, nullptr };
                    jgb_debug("add loop. { name = %s, id = %d }", name, i);
                    app.worker_.push_back(w);
                }
                else
                {
                    break;
                }
            }
        }
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
            jgb_bug("{ w = %p }", w);
            return;
        }

        jgb_debug("xxx");
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
            jgb_debug("{ size = %lu }", app->worker_.size());
            if(app->worker_.size())
            {
                struct worker& w = app->worker_.front();
                struct core_worker cw;
                w.thread_ = new boost::thread(cw, &w);
            }
        }
    }

    return JGB_ERR_FAIL;
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
            if(app->api_
                    && app->api_->task)
            {
                // TODO
                jgb_info("stop task. { path = \"%s\", app = %s }", path, name.c_str());
                return 0;
            }
        }
    }
    jgb_fail("start task. { path = \"%s\" }", path);
    return JGB_ERR_FAIL;
}

}
