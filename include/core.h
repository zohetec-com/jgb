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
#include <jgb/buffer.h>
#include <jgb/app.h>
#include <jgb/schema.h>
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

    void set_user(void* user);
    void* get_user();

    config* get_config();
    instance* get_instance();

    reader* get_reader(int index);
    writer* get_writer(int index);

    int id_;
    task* task_;
    bool run_;
    bool exited_;  // 线程是否已结束循环。
    bool normal_; // 线程的结束状态：true-正常; false-异常
    int64_t looped_;
    std::string worker_id_;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class task
{
public:
    task(instance* instance);
    ~task();

    int start();
    int stop();

    std::vector<reader*> readers_;
    std::vector<writer*> writers_;

    instance* instance_;
    std::vector<worker> workers_;
    worker* dummy_worker_;
    bool run_; // 控制线程：true-运行; false-结束
    enum task_state state_;

    // true - 发送 SIGUSR1 信号终止线程，false - 等待线程自然退出。
    bool send_kill_;

private:
    int start_single();
    int stop_single();

    int start_multiple();
    int stop_multiple();

    int init_io_readers();
    int init_io_writers();

    int init_io();
    void release_io();
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

    void set_user(void* user);
    void* get_user();

    app* app_;
    config* conf_;
    bool normal_;
    task* task_;
    // 实例的编号，从 0 开始。
    int id_;
    // 用户数据
    void* user_;

    // 从配置反查实例对象。
    static instance* get_instance(config* conf);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class app
{
public:
    app(const char* name, jgb_api_t* api, config* conf, schema* schema);
    ~app();

    int init();
    void release();

    std::string name_;
    jgb_api_t* api_;
    config* conf_;
    schema* schema_;
    std::vector<instance*> instances_;
    bool normal_; // is init ok

    // 当前使用的应用接口的版本。
    static const int current_api_interface_version = CURRENT_API_VERSION();
    // 从配置反查应用对象。
    static app* get_app(config* conf);
    // 查询应用配置所包含的实例配置的数量。
    int get_instance_count();

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
