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
      normal_(false),
      thread_(nullptr)
{
}

task::task(instance *instance)
    : instance_(instance),
      run_(false),
      state_(task_state_idle)
{
    jgb_assert(instance_);
    app* app = instance_->app_;
    workers_.resize(0);
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
                workers_.push_back(worker(i, this));
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

instance::instance(app* app, config* conf)
    : app_(app),
      conf_(conf),
      normal_(false),
      task_(this)
{
}

int instance::create()
{
    jgb_api_t* api_ = app_->api_;
    int r = 0;
    if(api_
            && api_->create)
    {
        r = api_->create(conf_);
    }
    normal_ = !r;
    return 0;
}

void instance::destroy()
{
    jgb_api_t* api_ = app_->api_;
    if(api_
            && api_->destroy)
    {
        api_->destroy(conf_);
    }
}

int instance::start()
{
    if(!normal_)
    {
        return JGB_ERR_DENIED;
    }
    return task_.start();
}

int instance::stop()
{
    if(!normal_)
    {
        return JGB_ERR_DENIED;
    }
    return task_.stop();
}

app::app(const char* name, jgb_api_t* api, config* conf)
    : name_(name),
      api_(api),
      conf_(conf),
      normal_(false)
{
    int r;

    instances_.resize(0);
    for(int i=0; ;i++)
    {
        std::string path = "/" + std::string(name) + "/instance[" + std::to_string(i) + ']';
        config* inst_conf;
        r = conf->get(path.c_str(), &inst_conf);
        if(!r)
        {
            instances_.push_back(instance(this, inst_conf));
        }
        else
        {
            break;
        }
    }
    if(!instances_.size())
    {
        instances_.push_back(instance(this, conf));
    }
}

int app::init()
{
    if(api_)
    {
        if(api_->version != current_api_interface_version)
        {
            jgb_fail("invalid api version. { api.version = %x, required = %x }",
                     api_->version, current_api_interface_version);
            return JGB_ERR_NOT_SUPPORT;
        }

        if(api_->init)
        {
            int r = api_->init(conf_);
            if(!r)
            {
                jgb_ok("install %s. { desc = \"%s\" }", name_.c_str(), api_->desc);
            }
            else
            {
                jgb_fail("install %s. { init() = %d }", name_.c_str(), r);
                return JGB_ERR_FAIL;
            }
        }
    }
    normal_ = true;
    return 0;
}

void app::release()
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

    app* papp = find(name);
    if(papp)
    {
        jgb_warning("app already exist. { name = %s, desc = %s }", name, papp->api_->desc);
        return JGB_ERR_IGNORED;
    }

    jgb_info("install app. { name = %s }", name);

    std::string conf_file_path = std::string(conf_dir_) + '/' + name + ".json";
    config* conf = config_factory::create(conf_file_path.c_str());
    app_conf_->add(name, conf);
    //jgb_debug("{ name = %s, conf = %p }", name, conf);

    app_.push_back(app(name, api, conf));
    app& app = app_.back();
    app.init();
    if(app.normal_)
    {
        for (auto & i : app.instances_)
        {
            i.create();
        }
    }

    return 0;
}

app* core::find(const char* name)
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

int core::start(const char* name, int idx)
{
    app* app = find(name);
    if(app && idx < (int) app->instances_.size())
    {
        return app->instances_[idx].start();
    }

    return JGB_ERR_INVALID;
}

int core::stop(const char* name, int idx)
{
    app* app = find(name);
    if(app && idx < (int) app->instances_.size())
    {
        return app->instances_[idx].stop();
    }

    return JGB_ERR_INVALID;
}

}
