#ifndef APP_H_20250331
#define APP_H_20250331

#include "config.h"
#include "app.h"
#include <list>
#include <boost/thread.hpp>

namespace jgb
{

struct app;

enum worker_state
{
    worker_state_idle,
    worker_state_running,
    worker_state_exiting,
    worker_state_aborted
};

struct worker
{
    int id_;
    struct app* app_;
    bool normal_;
    boost::thread* thread_;
};

struct app
{
    std::string name_;
    jgb_app_t* api_;
    config* conf_;
    std::list<worker> worker_;
};

class core
{
public:
    static core* get_instance();
    int set_conf_dir(const char* dir);
    int install(const char* name, jgb_app_t* api = nullptr);

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
