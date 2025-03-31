#include "config_factory.h"
#include "app.h"
#include "error.h"
#include <string>

namespace jgb
{
app::app()
{
    conf_dir = "/etc/jgb";
}

app* app::get_instance()
{
    static app a;
    return &a;
}

int app::set_conf_dir(const char* dir)
{
    if(!dir)
    {
        return JGB_ERR_INVALID;
    }

    // TODO: 检查目录是否存在？
    conf_dir = dir;
    jgb_notice("jgb setting changed: { conf_dir = %s }", conf_dir);

    return 0;
}

int app::install(const char* name, jgb_app_callback_t* app_callback)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    if(exist(name))
    {
        jgb_warning("app already exist. { name = %s }", name);
        return JGB_ERR_IGNORED;
    }

    std::string conf_file_path = std::string(conf_dir) + '/' + name + ".json";
    config* conf = config_factory::create(conf_file_path.c_str());
    app_conf_.add(name, conf);
    //jgb_debug("{ name = %s, conf = %p }", name, conf);

    if(app_callback)
    {
        if(app_callback->version != current_app_callback_version)
        {
            jgb_fail("invalid app version. { app.version = %x, required = %x }",
                     app_callback->version, current_app_callback_version);
            return JGB_ERR_NOT_SUPPORT;
        }

        if(app_callback->init)
        {
            int r = app_callback->init(conf);
            if(!r)
            {
                jgb_ok("install %s", name);
            }
            else
            {
                jgb_fail("install %s. { init() = %d }", name, r);
            }
        }
    }

    return 0;
}

bool app::exist(const char* name)
{
    for(auto it = app_info_.cbegin(); it != app_info_.end(); ++it)
    {
        if(!strcmp(name, (*it).name))
        {
            return true;
        }
    }
    return false;
}

}
