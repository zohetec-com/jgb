#include "config_factory.h"
#include "core.h"
#include "error.h"
#include "helper.h"
#include <string>

namespace jgb
{
core::core()
{
    conf_dir = "/etc/jgb";
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
    conf_dir = dir;
    jgb_notice("jgb setting changed: { conf_dir = %s }", conf_dir);

    return 0;
}

int core::install(const char* name, jgb_app_t* app)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    struct ap* ap = find(name);
    if(ap)
    {
        jgb_warning("app already exist. { name = %s, desc = %s }", name, ap->app->desc);
        return JGB_ERR_IGNORED;
    }

    std::string conf_file_path = std::string(conf_dir) + '/' + name + ".json";
    config* conf = config_factory::create(conf_file_path.c_str());
    app_conf_->add(name, conf);
    //jgb_debug("{ name = %s, conf = %p }", name, conf);

    struct ap ap_x {name, app, conf};
    ap_.push_back(ap_x);

    if(app)
    {
        if(app->version != current_app_interface_version)
        {
            jgb_fail("invalid app version. { app.version = %x, required = %x }",
                     app->version, current_app_interface_version);
            return JGB_ERR_NOT_SUPPORT;
        }

        if(app->init)
        {
            int r = app->init(conf);
            if(!r)
            {
                jgb_ok("install %s. { desc = \"%s\" }", name, app->desc);
            }
            else
            {
                jgb_fail("install %s. { init() = %d }", name, r);
            }
        }
    }

    return 0;
}

struct ap* core::find(const char* name)
{
    for(auto it = ap_.begin(); it != ap_.end(); ++it)
    {
        if(!strcmp(name, (*it).name.c_str()))
        {
            return &(*it);
        }
    }
    return nullptr;
}

int core::start(const char* path)
{
    std::string base;
    int r;
    const char* s = path;
    const char* e;

    r = jgb::jpath_parse(&s, &e);
    if(!r)
    {
        std::string name(s, e);
        struct ap* ap;
        ap = find(name.c_str());
        if(ap)
        {
            if(ap->app
                    && ap->app->task)
            {
                // TODO
                jgb_info("start task. { path = \"%s\", app = %s }", path, name.c_str());
                return 0;
            }
        }
    }
    jgb_fail("start task. { path = \"%s\" }", path);
    return JGB_ERR_FAIL;
}

}
