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
        integer,
        real,
        string,
        object
    };

    value(data_type type, int len = 1, bool is_array = false, bool is_bool = false)
    {
        jgb_assert(type == data_type::integer
                   || type == data_type::real
                   || type == data_type::string
                   || type == data_type::object);

        if(len > object_len_max)
        {
            jgb_warning("数组长度: %d，超限！将截断处理！", len);
            len = object_len_max;
        }

        if(len > 0)
        {
            int_ = new int64_t[len];
            jgb_assert(int_);
        }

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
    // 数组的 valid：必须初始化全部元素。

    int len_;
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
