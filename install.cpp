#include <map>
#include "debug.h"
#include "error.h"

// 库文件标识到动态库句柄
static const char* conf_dir = NULL;

extern "C" int jgb_set_conf_dir(const char* d)
{
    if(!d)
    {
        return JGB_ERR_INVALID;
    }

    if(conf_dir)
    {
        jgb_warning("JGB conf dir has been set and not allow to be changed to %s", conf_dir);
        return JGB_ERR_DENIED;
    }

    conf_dir = d;
    jgb_info("JGB conf dir: %s", d);

    return 0;
}

static int create_app_conf(const char* appname)
{
    return 0;
}

static int app_init(const char* appname, const char* libfile)
{
    return 0;
}

extern "C" int jgb_install(const char* appname, const char* libfile)
{
    jgb_info("install %s, from %s", appname, libfile);
    create_app_conf(appname);
    app_init(appname, libfile);
    return 0;
}
