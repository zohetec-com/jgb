#ifndef APP_H_20250331
#define APP_H_20250331

#include "config.h"
#include "app.h"
#include <list>

namespace jgb
{

struct ap
{
    std::string name;
    jgb_app_t* app;
    config* conf;
};

class core
{
public:
    static core* get_instance();
    int set_conf_dir(const char* dir);
    int install(const char* name, jgb_app_t* app = nullptr);

    int start(const char* path);
    int stop(const char* path);

    struct ap* find(const char* name);

    config* app_conf_;
    std::list<ap> ap_;
    const char* conf_dir;

    static const int current_app_interface_version = CURRENT_APP_VERSION();

private:
    core();
    ~core();
};

} // namespace jgb

#endif // APP_H
