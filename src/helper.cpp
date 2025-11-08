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

enum class jpath_parse_state
{
    init,
    s1, // got '/'
    s2, // got '['
    s3, // got others
};

int jpath_parse(const char** start, const char** end)
{
    if(start && *start)
    {
        const char* p = *start;
        jpath_parse_state state = jpath_parse_state::init;

        while(true)
        {
            switch(state)
            {
            case jpath_parse_state::init:
                switch(*p)
                {
                case '/':
                    state = jpath_parse_state::s1;
                    ++ p;
                    break;
                case '[':
                    state = jpath_parse_state::s2;
                    ++ p;
                    break;
                case ']':
                    return JGB_ERR_INVALID;
                case '\0':
                    if(end)
                    {
                        *end = p;
                    }
                    return 0;
                default:
                    state = jpath_parse_state::s3;
                    ++ p;
                    break;
                }
                break;
            case jpath_parse_state::s1:
                switch(*p)
                {
                case '/':
                    ++ p;
                    break;
                case '[':
                    *start = p;
                    state = jpath_parse_state::s2;
                    ++ p;
                    break;
                case ']':
                    return JGB_ERR_INVALID;
                case '\0':
                    *start = p - 1;
                    if(end)
                    {
                        *end = p;
                    }
                    return 0;
                default:
                    *start = p;
                    state = jpath_parse_state::s3;
                    ++ p;
                    break;
                }
                break;
            case jpath_parse_state::s2:
                switch(*p)
                {
                case '/':
                case '[':
                case '\0':
                    return JGB_ERR_INVALID;
                case ']':
                    if(end)
                    {
                        *end = p + 1;
                    }
                    return 0;
                default:
                    ++ p;
                    break;
                }
                break;
            case jpath_parse_state::s3:
                switch(*p)
                {
                    case '/':
                    case '[':
                    case '\0':
                        if(end)
                        {
                            *end = p;
                        }
                        return 0;
                    case ']':
                        return JGB_ERR_INVALID;
                    default:
                        ++ p;
                        break;
                }
                break;
            }
        }
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
    usleep(ms*1000);
    //信号对 boost::this_thread::sleep_for 无效。
    //boost::this_thread::sleep_for(boost::chrono::milliseconds(ms));
}

int put_string(char* buf, int len, int& offset, const char* format, ...)
{
    if(buf
        && len > 0
        && offset >= 0
        && offset < len)
    {
        va_list args;
        va_start(args, format);
        int ret = vsnprintf(buf + offset, len - offset, format, args);
        va_end(args);
        if(ret > 0)
        {
            offset += ret;
            if(offset > len)
            {
                offset = len;
                return JGB_ERR_LIMIT;
            }
            return 0;
        }
    }
    return JGB_ERR_FAIL;
}

int path_get_part(const char** start, const char** end)
{
    jgb_assert(start);
    jgb_assert(*start);
    jgb_assert(end);

    const char* s = *start;

    // skip leading '/'
    while (*s == '/')
    {
        s++;
    }

    if(*s == '\0')
    {
        return JGB_ERR_NOT_FOUND;
    }

    const char* p = s;
    while (*p && *p != '/')
    {
        p++;
    }
    jgb_assert(p != s);

    *start = s;
    *end = p;
    return 0;
}

} // namespace jgb
