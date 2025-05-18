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
#include "error.h"
#include "log.h"
#include "helper.h"
#include <math.h>
#include <stdexcept>
#include <string.h>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

namespace jgb
{

int str_to_index(int& idx, const char* s, const char* e)
{
    if(s)
    {
        if(!e)
        {
            e = s + strlen(s) - 1;
        }
        //jgb_debug("{ e - s = %d }", (int)(e - s));
        if(e > s + 1)
        {
            if(*s == '[' && *e == ']')
            {
                std::string str(s + 1, e);
                //jgb_debug("{ str = %s }", str.c_str());
                return stoi(str, idx);
            }
        }
    }
    return JGB_ERR_INVALID;
}

// 如 path = "/a/b/c[2]" 返回 path + 6
static const char* get_last_index(const char* path)
{
    const char* s = path;

    if(s && *s != '\0')
    {
        const char* p = path + strlen(path) - 1;
        if(*p == ']')
        {
            -- p;
            while(p >= s)
            {
                if(*p == '[')
                {
                    return p;
                }
                -- p;
            }
        }
    }
    return nullptr;
}

int get_base_index(const char* path, std::string& base, int& idx)
{
    if(!path)
    {
        return JGB_ERR_INVALID;
    }

    const char* sidx = get_last_index(path);
    if(sidx)
    {
        if(!str_to_index(idx, sidx))
        {
            base = std::string(path, sidx);
            return 0;
        }
        return JGB_ERR_INVALID;
    }
    else
    {
        idx = 0;
        base = std::string(path);
        return 0;
    }
}

// 输入不能包含有空格！
int jpath_parse(const char** start, const char** end)
{
    if(start && *start)
    {
        const char* p = *start;
        // 跳过开始的 "///"
        while(*p == '/' && *p != '\0')
        {
            ++ p;
        }
        jgb_assert(*p != '/');
        *start = p;
        if(end)
        {
            const char* s = p;
            while(true)
            {
                if(*p == '/'
                        || *p == '\0')
                {
                    break;
                }

                if(*s == '[')
                {
                    if(*p == ']')
                    {
                        ++ p;
                        break;
                    }
                }
                else
                {
                    if(*p == '[')
                    {
                        break;
                    }
                }

                ++ p;
            }
            *end = p;
        }
        return 0;
    }
    else
    {
        return JGB_ERR_INVALID;
    }
}

// https://www.geeksforgeeks.org/compare-float-and-double-while-accounting-for-precision-loss/
bool is_equal(double a, double b, double epsilon)
{
    return fabs(a - b) < epsilon;
}

int stoi(const std::string& str, int& v)
{
    std::size_t pos{};
    bool fail = false;

    try
    {
        v = std::stoi(str, &pos, 0);
    }
    catch (...)
    {
        jgb_mark();
        fail = true;
    }

    if(!fail)
    {
        if(str[pos] == '\0')
        {
            return 0;
        }
        jgb_debug("{ pos = %lu }", pos);
    }

    jgb_debug("{ str = %s}", str.c_str());
    return JGB_ERR_INVALID;
}

int stoll(const std::string& str, int64_t& v)
{
    std::size_t pos{};
    bool fail = false;

    try
    {
        v = std::stoll(str, &pos, 0);
    }
    catch (...)
    {
        jgb_mark();
        fail = true;
    }

    if(!fail)
    {
        if(str[pos] == '\0')
        {
            return 0;
        }
        jgb_debug("{ pos = %lu }", pos);
    }

    jgb_debug("{ str = %s}", str.c_str());
    return JGB_ERR_INVALID;
}

int stod(const std::string& str, double& v)
{
    std::size_t pos{};
    bool fail = false;

    try
    {
        v = std::stod(str, &pos);
    }
    catch (...)
    {
        jgb_mark();
        fail = true;
    }

    if(!fail)
    {
        if(str[pos] == '\0')
        {
            return 0;
        }
        jgb_debug("{ pos = %lu }", pos);
    }

    jgb_debug("{ str = %s}", str.c_str());
    return JGB_ERR_INVALID;
}

void sleep(int ms)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(ms));
}

} // namespace jgb
