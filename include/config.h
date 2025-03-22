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
#ifndef CONFIG_H_20250318
#define CONFIG_H_20250318

#include <memory>
#include <list>
#include <inttypes.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "debug.h"

namespace jgb
{
class value;
class pair;
class config;

static const int object_len_max = 1024;

class value
{
public:
    enum class data_type
    {
        none,
        integer,
        real,
        string,
        object
    };

    value(data_type type, int len = 1, bool is_array = false, bool is_bool = false)
    {
        if(len > object_len_max)
        {
            jgb_warning("请求创建数组，长度超限，已截断处理！{ len = %d，object_len_max = %d }", len, object_len_max);
            len = object_len_max;
        }

        if(len > 0)
        {
            int_ = new int64_t[len];
            jgb_assert(int_);
        }
        else
        {
            int_ = nullptr;
        }

        type_ = type;
        len_ = len;
        if(len > 1)
        {
            is_array_ = true;
        }
        else
        {
            is_array_ = is_array;
        }
        is_bool_ = is_bool;
    }

    int reinit(data_type type, int len, bool is_array, bool is_bool)
    {
        if(type_ == data_type::none)
        {
            if(len > 0)
            {
                if(len > object_len_max)
                {
                    jgb_warning("请求创建数组，长度超限，已截断处理！请求 %d，最大 %d", len, object_len_max);
                    len = object_len_max;
                }

                int_ = new int64_t[len];
                jgb_assert(int_);

                type_ = type;
                len_ = len;
                if(len > 1)
                {
                    is_array = true;
                }
                else
                {
                    is_array_ = is_array;
                }
                is_bool_ = is_bool;
            }
            else
            {
                return JGB_ERR_INVALID;
            }
        }
        else
        {
            // TODO：需要支持重新初始化吗？
            return JGB_ERR_DENIED;
        }
    }

    ~value();

    friend std::ostream& operator<<(std::ostream& os, const value* val);

    bool valid()
    {
        return valid_;
    }

    data_type type()
    {
        return type_;
    }

    int len()
    {
        return len_;
    }

    int64_t integer()
    {
        jgb_assert(type_ == data_type::integer);
        jgb_assert(valid_);
        jgb_assert(len_ > 0);
        return int_[0];
    }

    union
    {
        int64_t* int_;
        double* real_;
        const char** str_;
        config** conf_;
    };

    data_type type_;
    // 是否支持改变长度？
    //  -- 预料大部分的场景不需要，但仍应该提供支持。
    //  -- 考虑到大部分的场景不需要，未减小资源占用，避免直接使用 std::vector。


    int len_;

    // 指示 int_/real_ 中存储的内容是否有效：
    //   - true：有效，内容可以使用。
    //   - false：无效，内容不可使用。
    // 应用场景1：
    //   - 在规格文件中定义参数 p 的类型为 int，在配置文件中设置 p 初值为 null。
    //   - 期望：所创建的 p 参数的 valid_ 应为 false。
    bool valid_;

    // TODO:优先采用 schema 定义。
    // 处理长度为 1 的情况。
    bool is_array_;
    bool is_bool_;
};

class pair
{
public:
    pair(const char* name, value* value)
    {
        // 疑问：对 name 和 value 区别对待，这么做合适吗？
        name_ = strdup(name);
        value_ = value;
    }

    ~pair()
    {
        free((void*) name_);
        delete value_;
    }

    friend std::ostream& operator<<(std::ostream& os, const pair* pr);

public:
    const char* name_;
    value* value_;
};

class config
{
public:
    ~config()
    {
        for (auto & i : conf_)
        {
            delete i;
        }
    }

    pair* find(const char* name)
    {
        if(name)
        {
            for (auto it = conf_.begin(); it != conf_.end(); ++it)
            {
                if(!strcmp(name, (*it)->name_))
                {
                    return *it;
                }
            }
        }
        return nullptr;
    }

    friend std::ostream& operator<<(std::ostream& os, const config* conf);

    //int set_value(const char* path, const value& val);

public:
    std::list<pair*> conf_;
};

}

#endif // CONFIG_H
