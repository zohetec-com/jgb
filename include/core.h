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
#ifndef APP_H_20250331
#define APP_H_20250331

#include <jgb/config.h>
#include <jgb/app.h>
#include <vector>
#include <list>

namespace jgb
{

class app;
class task;
class instance;

enum task_state
{
    task_state_idle,
    task_state_running,
    task_state_aborted
};

class worker
{
public:
    worker(int id = 0, task* task = nullptr);

    int start();
    int stop();

    std::string get_thread_id();

    int id_;
    task* task_;
    bool run_;
    bool exited_;  // 线程是否已结束循环。
    bool normal_; // 线程的结束状态：true-正常; false-异常
    int64_t looped_;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class task
{
public:
    task(instance* instance);

    int start();
    int stop();

    instance* instance_;
    std::vector<worker> workers_;
    bool run_; // 控制线程：true-运行; false-结束
    enum task_state state_;

private:
    int start_single();
    int stop_single();

    int start_multiple();
    int stop_multiple();
};

class instance
{
public:
    instance(int id, app* app = nullptr, config* conf = nullptr);
    ~instance();

    int create();
    void destroy();

    int start();
    int stop();

    void* get_mutex();
    void lock_shared();
    void unlock_shared();
    void lock();
    void unlock();

    app* app_;
    config* conf_;
    bool normal_;
    task* task_;
    // 实例的编号，从 0 开始。
    int id_;
    // 用户数据
    void* user_;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class app
{
public:
    app(const char* name, jgb_api_t* api, config* conf);
    ~app();

    int init();
    void release();

    std::string name_;
    jgb_api_t* api_;
    config* conf_;
    std::vector<instance*> instances_;
    bool normal_; // is init ok

    // 当前使用的应用接口的版本。
    static const int current_api_interface_version = CURRENT_API_VERSION();

private:
    void create_instances();
};

class core
{
public:
    static core* get_instance();

    config* root_conf();
    int set_conf_dir(const char* dir);
    const char* conf_dir();

    int install(const char* name, jgb_api_t* api = nullptr);
    // uninstall all app.
    void uninstall_all();

    int start(const char* name, int idx = 0);
    int stop(const char* name, int idx = 0);

    app* find(const char* name);

    // 保存全部应用的相关信息：逻辑接口、配置入口、任务。
    std::list<app*> app_;

private:
    core();
    ~core();

    int check(const char* name);

    // 保存全部应用的配置，整体为树状结构。
    config* app_conf_;
    // 存放配置文件的目录。
    const char* conf_dir_;
};

} // namespace jgb

#endif // APP_H
