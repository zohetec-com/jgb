/*
 * jgb - a framework for linux media streaming application
 *
 * Copyright (C) 2025 Beijing Zohetec Co., Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "config_factory.h"
#include "core.h"
#include "error.h"
#include "helper.h"
#include <string>
#include <dlfcn.h>
#include <boost/thread.hpp>

namespace jgb
{

struct core_worker
{
    void operator()(struct worker* w)
    {
        //jgb_function();

        if(!w)
        {
            jgb_bug();
            return;
        }

        jgb_assert(!w->exited_);
        jgb_assert(w->normal_);
        jgb_assert(w->task_);
        jgb_assert(w->task_->instance_);
        jgb_assert(w->task_->instance_->app_);
        jgb_assert(w->task_->instance_->app_->api_);
        jgb_assert(w->task_->instance_->app_->api_->loop);
        jgb_assert(w->task_->instance_->app_->api_->loop->loops);
        jgb_assert(w->task_->instance_->app_->api_->loop->loops[w->id_]);

        int r;
        jgb_loop_t* loop = w->task_->instance_->app_->api_->loop;
        bool single = w->task_->workers_.size() == 1;

        w->looped_ = 0L;
        if(single)
        {
            jgb_assert(!w->id_);
            if(loop->setup)
            {
                r = loop->setup(w);
                if(r)
                {
                    w->exited_ = true;
                    w->normal_ = false;
                    return;
                }
            }
        }

        while(w->run_)
        {
            r = loop->loops[w->id_](w);
            ++ w->looped_;
            if(r)
            {
                w->normal_ = false;
                jgb_fail("{ r = %d, id = %d }", r, w->id_);
                break;
            }
        }
        w->exited_ = true;

        if(single)
        {
            if(loop->exit)
            {
                loop->exit(w);
            }
        }

        jgb_debug("loop exit. { app = %s, inst id = %d, worker id = %d, looped = %ld }",
                  w->task_->instance_->app_->name_.c_str(),
                  w->task_->instance_->id_,
                  w->id_,
                  w->looped_);
    }
};

struct worker::Impl
{
    boost::thread* thread_;
};

worker::worker(int id, task* task)
    : id_(id),
      task_(task),
      run_(false),
      exited_(false),
      normal_(true),
      pimpl_(new Impl())
{
}

int worker::start()
{
    if(!pimpl_->thread_)
    {
        struct core_worker cw;
        run_ = true;
        exited_ = false;
        normal_ = true;
        pimpl_->thread_ = new boost::thread(cw, this);
        return 0;
    }
    else
    {
        jgb_bug();
        return JGB_ERR_FAIL;
    }
}

int worker::stop()
{
    if(pimpl_->thread_)
    {
        run_ = false;
        jgb_debug("join thread. { id = %d, thread id = %s }", id_, get_thread_id().c_str());
        pimpl_->thread_->join();
        jgb_debug("join thread done. { id = %d }", id_);
        delete pimpl_->thread_;
        pimpl_->thread_ = nullptr;
    }
    return 0;
}

std::string worker::get_thread_id()
{
    if(pimpl_->thread_)
    {
        // https://stackoverflow.com/questions/61203655/how-to-printf-stdthis-threadget-id-in-c
        std::ostringstream oss;
        oss << pimpl_->thread_->get_id();
        return oss.str();
    }
    else
    {
        return std::string();
    }
}

void worker::set_user(void* user)
{
    task_->instance_->user_ = user;
}

void* worker::get_user()
{
    return task_->instance_->user_;
}

config* worker::get_config()
{
    return task_->instance_->conf_;
}

reader* worker::get_reader(int index)
{
    if(index >=0 && index < (int) task_->readers_.size())
    {
        return task_->readers_[index];
    }
    jgb_warning("invalid reader index. { index = %d }", index);
    return nullptr;
}

writer* worker::get_writer(int index)
{
    if(index >=0 && index < (int) task_->writers_.size())
    {
        return task_->writers_[index];
    }
    jgb_warning("invalid writer index. { index = %d }", index);
    return nullptr;
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
        if(loop->loops)
        {
            for(int i=0; loop->loops[i]; i++)
            {
                jgb_debug("add worker. { app.name = %s, id = %d }", app->name_.c_str(), i);
                workers_.push_back(worker(i, this));
            }
        }
    }
}

int task::start_single()
{
    jgb_assert(run_);
    jgb_assert(workers_.size() == 1);
    workers_[0].start();
    return 0;
}

int task::start_multiple()
{
    jgb_assert(run_);
    jgb_assert(workers_.size() > 1);
    jgb_assert(instance_);
    jgb_assert(instance_->app_);
    jgb_assert(instance_->app_->api_);
    jgb_assert(instance_->app_->api_->loop);

    jgb_loop_t* loop = instance_->app_->api_->loop;
    int r;

    if(loop->setup)
    {
        r = loop->setup(&workers_[0]);
        if(r)
        {
            // TODO: 补充参数
            jgb_fail("task setup.");
            return JGB_ERR_FAIL;
        }
    }

    for(auto i = workers_.begin(); i != workers_.end(); ++i)
    {
        i->start();
    }

    return 0;
}

// TODO: 线程安全。
int task::start()
{
    //jgb_debug("{ workers_.size() = %lu }", workers_.size());
    if(workers_.size() > 0)
    {
        if(state_ == task_state_running)
        {
            return 0;
        }
        else if(state_ == task_state_idle || state_ == task_state_aborted)
        {
            jgb_info("start task. { name = %s, id = %d }",
                      instance_->app_->name_.c_str(), instance_->id_);

            init_io();

            // 启动任务
            int r;
            run_ = true;
            if(workers_.size() > 1)
            {
                r =  start_multiple();
            }
            else
            {
                r = start_single();
            }
            if(!r)
            {
                state_ = task_state_running;
            }
            else
            {
                release_io();
                // todo: 补充参数
                jgb_fail("start task.");
            }
            return r;
        }
        else
        {
            // TODO: 等待状态切换？
            return JGB_ERR_RETRY;
        }
    }
    else
    {
        return JGB_ERR_IGNORED;
    }
}

int task::stop_single()
{
    jgb_assert(workers_.size() == 1);
    return workers_[0].stop();
}

int task::stop_multiple()
{
    for(auto i = workers_.rbegin(); i != workers_.rend(); ++i)
    {
        i->stop();
    }

    jgb_assert(instance_);
    jgb_assert(instance_->app_);
    jgb_assert(instance_->app_->api_);
    jgb_assert(instance_->app_->api_->loop);

    jgb_loop_t* loop = instance_->app_->api_->loop;
    if(loop->exit)
    {
        loop->exit(&workers_[0]);
    }

    return 0;
}

// TODO: 线程安全。
int task::stop()
{
    if(workers_.size() > 0)
    {
        if(state_ == task_state_running)
        {
            jgb_debug("stop task. { name = %s, id = %d }",
                      instance_->app_->name_.c_str(), instance_->id_);

            int r;
            run_ = false;
            if(workers_.size() > 1)
            {
                r = stop_multiple();
            }
            else
            {
                r = stop_single();
            }
            if(!r)
            {
                release_io();
                state_ = task_state_idle;
            }
            else
            {
                // TODO: 补充参数
                jgb_fail("stop task.");
            }
            return r;
        }
        else if(state_ == task_state_idle || state_ == task_state_aborted)
        {
            return 0;
        }
        else
        {
            // TODO: 该如何处理？
            return JGB_ERR_RETRY;
        }
    }
    else
    {
        return JGB_ERR_IGNORED;
    }
}

int task::init_io_readers()
{
    int r;
    value* val;

    // 打开读者
    r = instance_->conf_->get("task/readers", &val);
    if(!r)
    {
        for(int i=0; i<val->len_; i++)
        {
            std::string id;
            int r = val->conf_[i]->get("buf_id", id);
            if(!r && !id.empty())
            {
                buffer* buf = buffer_manager::get_instance()->add_buffer(id);
                if(buf)
                {
                    reader* rd = buf->add_reader();
                    if(rd)
                    {
                        readers_.push_back(rd);
                    }
                    jgb_assert(rd);
                }
                jgb_assert(buf);
            }
        }
    }
    return 0;
}

int task::init_io_writers()
{
    int r;
    value* val;

    // 打开写者
    r = instance_->conf_->get("task/writers", &val);
    if(!r)
    {
        for(int i=0; i<val->len_; i++)
        {
            std::string id;
            int r = val->conf_[i]->get("buf_id", id);
            if(!r && !id.empty())
            {
                buffer* buf = buffer_manager::get_instance()->add_buffer(id);
                if(buf)
                {
                    writer* wr = buf->add_writer();
                    if(wr)
                    {
                        writers_.push_back(wr);

                        int sz;
                        r = val->conf_[i]->get("buf_size", sz);
                        if(!r)
                        {
                            buf->resize(sz);
                        }
                    }
                    jgb_assert(wr);
                }
                jgb_assert(buf);
            }
        }
    }
    return 0;
}

int task::init_io()
{
    init_io_readers();
    init_io_writers();
    return 0;
}

void task::release_io()
{
    // 关闭读者
    for(auto rd: readers_)
    {
        buffer* buf = rd->buf_;
        buf->remove_reader(rd);
        buffer_manager::get_instance()->remove_buffer(buf);
    }

    // 关闭写者
    for(auto wr: writers_)
    {
        buffer* buf = wr->buf_;
        buf->remove_writer(wr);
        buffer_manager::get_instance()->remove_buffer(buf);
    }
}

struct instance::Impl
{
    boost::shared_mutex rw_mutex;
};

void* instance::get_mutex()
{
    jgb_assert(pimpl_);
    return &pimpl_->rw_mutex;
}

void instance::lock_shared()
{
    jgb_assert(pimpl_);
    pimpl_->rw_mutex.lock_shared();
}

void instance::unlock_shared()
{
    jgb_assert(pimpl_);
    pimpl_->rw_mutex.unlock_shared();
}

void instance::lock()
{
    jgb_assert(pimpl_);
    pimpl_->rw_mutex.lock();
}

void instance::unlock()
{
    jgb_assert(pimpl_);
    pimpl_->rw_mutex.unlock();
}

instance::instance(int id, app* app, config* conf)
    : app_(app),
      conf_(conf),
      normal_(true),
      id_(id),
      user_(nullptr),
      pimpl_(new Impl())
{
    jgb_assert(app_);
    jgb_assert(conf_);
    task_ = new task(this);
}

instance::~instance()
{
    jgb_assert(task_);
    delete task_;
}

int instance::create()
{
    jgb_assert(app_);
    jgb_api_t* api_ = app_->api_;
    int r = 0;
    if(api_
            && api_->create)
    {
        conf_->create(".instance", reinterpret_cast<intptr_t>(this));
        r = api_->create(conf_);
    }
    normal_ = !r;
    return 0;
}

void instance::destroy()
{
    jgb_assert(app_);
    jgb_api_t* api_ = app_->api_;
    if(api_
            && api_->destroy)
    {
        api_->destroy(conf_);
    }
}

int instance::start()
{
    jgb_debug("start instance.{ name = %s, id = %d, normal_ = %d }", app_->name_.c_str(), id_, normal_);
    if(!normal_)
    {
        return JGB_ERR_DENIED;
    }
    return task_->start();
}

int instance::stop()
{
    if(!normal_)
    {
        return JGB_ERR_DENIED;
    }
    return task_->stop();
}

void instance::set_user(void* user)
{
    user_ = user;
}

void* instance::get_user()
{
    return user_;
}

void app::create_instances()
{
    jgb_assert(conf_);

    instances_.resize(0);
    for(int i=0; ;i++)
    {
        std::string path = "/instances[" + std::to_string(i) + ']';
        config* inst_conf;
        int r = conf_->get(path.c_str(), &inst_conf);
        if(!r)
        {
            instance* inst = new instance(i, this, inst_conf);
            instances_.push_back(inst);
        }
        else
        {
            break;
        }
    }
    if(!instances_.size())
    {
        instance* inst = new instance(0, this, conf_);
        instances_.push_back(inst);
    }
}
app::app(const char* name, jgb_api_t* api, config* conf)
    : name_(name),
      api_(api),
      conf_(conf),
      normal_(false)
{
    create_instances();
}

app::~app()
{
    // 前置条件：已经调用过 release()。
    jgb_assert(instances_.empty());
}

int app::init()
{
    jgb_assert(!normal_);
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
            conf_->create(".app", reinterpret_cast<intptr_t>(this));
            int r = api_->init(conf_);
            if(r)
            {
                jgb_fail("install %s. { init() = %d }", name_.c_str(), r);
                return JGB_ERR_FAIL;
            }
        }
        for (auto & i : instances_)
        {
            i->create();
        }
    }
    normal_ = true;
    if(api_ && api_->desc)
    {
        jgb_ok("app installed. { name = \"%s\", desc = \"%s\" }", name_.c_str(), api_->desc);
    }
    else
    {
        jgb_ok("app installed. { name = \"%s\" }", name_.c_str());

    }
    return 0;
}

void app::release()
{
    for (auto & i : instances_)
    {
        i->stop();
        i->destroy();
        // 在此执行删除可以吗？
        delete i;
    }
    instances_.clear();
    if(api_ && api_->release)
    {
        api_->release(conf_);
    }
}

app* app::get_app(config* conf)
{
    int64_t int_ptr;
    int r;
    r = conf->get(".app", int_ptr);
    if(!r)
    {
        return reinterpret_cast<app*>(int_ptr);
    }
    return nullptr;
}

instance* instance::get_instance(config* conf)
{
    int64_t int_ptr;
    int r;
    r = conf->get(".instance", int_ptr);
    if(!r)
    {
        return reinterpret_cast<instance*>(int_ptr);
    }
    return nullptr;
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

config* core::root_conf()
{
    return app_conf_;
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

const char* core::conf_dir()
{
    return conf_dir_;
}

int core::check(const char* name)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    app* papp = find(name);
    if(papp)
    {
        jgb_warning("app already exist. { name = \"%s\" }", name);
        return JGB_ERR_IGNORED;
    }

    return 0;
}

int core::install(const char* name, jgb_api_t* api)
{
    int r;

    r = check(name);
    if(!r)
    {
        std::string conf_file_path = std::string(conf_dir_) + '/' + name + ".json";
        config* conf = config_factory::create(conf_file_path.c_str());
        if(!conf)
        {
            conf = new config;
        }

        app* papp = new app(name, api, conf);
        app_conf_->create(name, papp->conf_);
        app_.push_back(papp);
        papp->init();
    }

    return r;
}

void core::uninstall_all()
{
    for(auto it = app_.rbegin(); it != app_.rend(); ++it)
    {
        (*it)->release();
        delete (*it);
    }
    app_.clear();
    app_conf_->clear();
}

app* core::find(const char* name)
{
    for(auto it = app_.begin(); it != app_.end(); ++it)
    {
        if(!strcmp(name, (*it)->name_.c_str()))
        {
            return (*it);
        }
    }
    return nullptr;
}

int core::start(const char* name, int idx)
{
    app* app = find(name);
    if(app && idx < (int) app->instances_.size())
    {
        return app->instances_[idx]->start();
    }

    return JGB_ERR_INVALID;
}

int core::stop(const char* name, int idx)
{
    app* app = find(name);
    if(app && idx < (int) app->instances_.size())
    {
        return app->instances_[idx]->stop();
    }

    return JGB_ERR_INVALID;
}

}
