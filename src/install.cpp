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
    (void) appname;
    return 0;
}

static int app_init(const char* appname, const char* libfile)
{
    (void) appname;
    (void) libfile;
    return 0;
}

extern "C" int jgb_install(const char* appname, const char* libfile)
{
    jgb_info("install %s, from %s", appname, libfile);
    create_app_conf(appname);
    app_init(appname, libfile);
    return 0;
}
