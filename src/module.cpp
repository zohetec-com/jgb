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
#include "core.h"
#include <dlfcn.h>
#include <string>

struct lib_info
{
    std::string file;
    void* handle;
};

std::list<struct lib_info> lib_info_list;

static int get_handle(const char* name, void** handle)
{
    for(auto it = lib_info_list.begin(); it != lib_info_list.end(); ++it)
    {
        if(it->file == name)
        {
            *handle = it->handle;
            return 0;
        }
    }
    return -1;
}

static void import(jgb::config* conf)
{
    void* handle = nullptr;

    // 加载全部库文件。
    // 为避免不确定性，只从最后加载的 lib 加载 app。
    for(int i=0;;i++)
    {
        std::string path = "library[" + std::to_string(i) + ']';
        const char* file;
        int r = conf->get(path.c_str(), &file);
        if(!r)
        {
            jgb_debug("{ i = %d, library = \"%s\" }", i, file);
            r = get_handle(file, &handle);
            if(r)
            {
                struct lib_info info;
                handle = dlopen(file, RTLD_NOW | RTLD_GLOBAL);
                if(handle)
                {
                    jgb_ok("load library. { file = \"%s\" }", file);
                }
                else
                {
                    jgb_fail("load library. { file = \"%s\", error = \"%s\" }", file, dlerror());
                }
                info.handle = handle;
                info.file = file;
                lib_info_list.push_back(info);
            }
            else
            {
                jgb_debug("library already loaded. { file = \"%s\" }", file);
            }
        }
        else
        {
            jgb_debug("load library done. { i = %d }", i);
            break;
        }
    }

    // 加载全部 app。
    for(int i=0;;i++)
    {
        std::string path = "name[" + std::to_string(i) + ']';
        const char* name;
        int r = conf->get(path.c_str(), &name);
        if(!r)
        {
            jgb_debug("load app. { i = %d, name = \"%s\" }", i, name);
            jgb_api_t* api = nullptr;
            if(handle)
            {
                 api = (jgb_api_t*) dlsym(handle, name);
            }
            jgb::core::get_instance()->install(name, api);
        }
        else
        {
            jgb_debug("load app done. { i = %d }", i);
            break;
        }
    }
}

static void release(void*)
{
    for(auto it = lib_info_list.rbegin(); it != lib_info_list.rend(); ++it)
    {
        if(it->handle)
        {
            // https://stackoverflow.com/questions/44627258/addresssanitizer-and-loading-of-dynamic-libraries-at-runtime-unknown-module
            // 为避免 AddressSanitizer 报告 "(<unknown module>)"，不卸载动态库文件。
#ifndef DEBUG
            dlclose(it->handle);
#endif
        }
    }
    lib_info_list.clear();
}

static int init(void* conf)
{
    jgb::config* c = (jgb::config*) conf;

    for(int i=0;;i++)
    {
        std::string path = "import[" + std::to_string(i) + "]";
        jgb::config* import_conf;
        int r = c->get(path.c_str(), &import_conf);
        if(!r)
        {
            jgb_debug("import. { path = \"%s\" }", path.c_str());
            import(import_conf);
        }
        else
        {
            break;
        }
    }

    return 0;
}

jgb_api_t module
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "import jgb app",
    .init = init,
    .release = release,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = nullptr
};
