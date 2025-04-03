#ifndef APP_H_20250331
#define APP_H_20250331

#include "config.h"
#include "app.h"
#include <vector>
#include <list>
#include <boost/thread.hpp>

namespace jgb
{

class app;
class task;

enum task_state
{
    task_state_idle,
    task_state_staring, // 在创建 worker 0 工作线程之前。
    task_state_running,
    task_state_exiting,
    task_state_aborted
};

class worker
{
public:
    worker(int id = 0, task* task = nullptr);

    int id_;
    struct task* task_;
    bool normal_; // 线程的结束状态：true-正常; false-异常
    boost::thread* thread_;
};

class task
{
public:
    task(struct app* app = nullptr);
    int start();
    int stop();

    struct app* app_;
    std::vector<worker> worker_;
    bool run_; // 控制线程：true-运行; false-结束
    enum task_state state_;
};

class app
{
public:
    app(const char* name, jgb_api_t* api, config* conf);

    std::string name_;
    jgb_api_t* api_;
    config* conf_;
    struct task task_;
};

class core
{
public:
    static core* get_instance();
    int set_conf_dir(const char* dir);
    int install(const char* name, jgb_api_t* api = nullptr);

    int start(const char* path);
    int stop(const char* path);

    struct app* find(const char* name);

    // 保存全部应用的配置，整体为树状结构。
    config* app_conf_;
    // 保存全部应用的相关信息：逻辑接口、配置入口、任务。
    std::list<app> app_;
    // 存放配置文件的目录。
    const char* conf_dir_;

    // 当前使用的应用接口版本。
    static const int current_api_interface_version = CURRENT_API_VERSION();

private:
    core();
    ~core();
};

} // namespace jgb

#endif // APP_H
