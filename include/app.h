#ifndef APP_H_20250331
#define APP_H_20250331

#include "config.h"
#include "app-callback.h"
#include <list>

namespace jgb
{

struct app_info
{
    const char* name;
    jgb_app_callback_t* callback;
    config* conf;
};

class app
{
public:
    static app* get_instance();
    int set_conf_dir(const char* dir);
    int install(const char* name, jgb_app_callback_t* app_callback = nullptr);
    bool exist(const char* name);

    config app_conf_;
    std::list<app_info> app_info_;
    const char* conf_dir;

private:
    app();
};

} // namespace jgb

#endif // APP_H
