#include "config_factory.h"
#include "core.h"
#include "error.h"
#include "helper.h"
#include <string>

namespace jgb
{

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

        jgb_assert(!w->normal_);
        jgb_assert(w->task_);
        jgb_assert(w->task_->instance_);
        jgb_assert(w->task_->instance_->app_);
        jgb_assert(w->task_->instance_->app_->api_);
        jgb_assert(w->task_->instance_->app_->api_->loop);
        jgb_assert(w->task_->instance_->app_->api_->loop->loops[w->id_]);

        jgb_loop_t* loop = w->task_->instance_->app_->api_->loop;
        task* task = w->task_;
        int r;

        if(!w->id_)
        {
            if(loop->setup)
            {
                r = loop->setup(w);
                if(r)
                {
                    return;
                }
            }

            for (auto & i : task->workers_)
            {
                if(i.id_)
                {
                    i.start();
                }
            }

            task->state_ = task_state_running;
        }

        while(task->run_)
        {
            r = loop->loops[w->id_](w);
            if(r)
            {
                jgb_fail("{ r = %d, id = %d }", r, w->id_);
                break;
            }
        }

        if(!r)
        {
            w->normal_ = true;
        }

        if(!w->id_)
        {
            for (auto & i : task->workers_)
            {
                if(i.id_)
                {
                    i.stop();
                }
            }

            if(loop->exit)
            {
                loop->exit(w);
            }

            task->state_ = task_state_exiting;
        }

        jgb_debug("loop exit. { id = %d }", w->id_);
    }
};

worker::worker(int id, task* task)
    : id_(id),
      task_(task),
      normal_(false),
      thread_(nullptr)
{
}

int worker::start()
{
    if(!thread_)
    {
        struct core_worker cw;
        normal_ = false;
        thread_ = new boost::thread(cw, this);

        return 0;
    }
    else
    {
        jgb_bug();
        return JGB_ERR_DENIED;
    }
}

int worker::stop()
{
    if(thread_)
    {
        jgb_assert(!task_->run_);
        jgb_debug("start jont thread. { id = %d, thread id = %s }", id_, get_thread_id().c_str());
        thread_->join();
        jgb_debug("jont thread done. { id = %d, thread id = %s }", id_, get_thread_id().c_str());
        delete thread_;
        thread_ = nullptr;
        return 0;
    }
    return 0;
}

std::string worker::get_thread_id()
{
    if(thread_)
    {
        // https://stackoverflow.com/questions/61203655/how-to-printf-stdthis-threadget-id-in-c
        std::ostringstream oss;
        oss << thread_->get_id();
        return oss.str();
    }
    else
    {
        return std::string();
    }
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
            && app->api_->loop)
    {
        jgb_loop_t* loop = app->api_->loop;
        for(int i=0;;i++)
        {
            if(loop->loops[i])
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
    if(!workers_.size())
    {
        return JGB_ERR_IGNORED;
    }

    if(state_ == task_state_running)
    {
        return 0;
    }
    else if(state_ == task_state_idle || state_ == task_state_aborted)
    {
        // 启动任务
        state_ = task_state_staring;
        run_ = true;
        return workers_[0].start();
    }
    else
    {
        return JGB_ERR_RETRY;
    }
}

int task::stop()
{
    if(!workers_.size())
    {
        return JGB_ERR_IGNORED;
    }
    if(state_ == task_state_running)
    {
        run_ = false;
        // 竞争！-- 工作线程也要修改 state_
        state_ = task_state_stopping;
        int r = workers_[0].stop();
        if(!r)
        {
            state_ = task_state_idle;
        }
        else
        {
            // ???
            jgb_assert(0);
        }
    }
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
        for (auto & i : instances_)
        {
            i.create();
        }
    }
    normal_ = true;
    return 0;
}

void app::release()
{
    for (auto & i : instances_)
    {
        i.stop();
        i.destroy();
    }
    if(api_ && api_->release)
    {
        api_->release(conf_);
    }
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

    return 0;
}

void core::uninstall_all()
{
    for(auto it = app_.rbegin(); it != app_.rend(); ++it)
    {
        it->release();
    }

    delete app_conf_;
    app_.clear();
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
