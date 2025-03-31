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
#include "config.h"
#include "error.h"
#include "debug.h"

namespace jgb
{

value::~value()
{
    if(len_ > 0)
    {
        if(type_ == data_type::string)
        {
            for(int i=0; i<len_; i++)
            {
                free((void*)str_[i]);
            }
        }
        else if(type_ == data_type::object)
        {
            for(int i=0; i<len_; i++)
            {
                delete conf_[i];
            }
        }
        delete [] int_;
    }
}

value::value(data_type type, int len, bool is_array, bool is_bool)
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
    valid_ = false;
}

pair::pair(const char* name, value* value)
{
    // 疑问：对 name 和 value 区别对待，这么做合适吗？
    name_ = strdup(name);
    value_ = value;
}

pair::~pair()
{
    free((void*) name_);
    delete value_;
}

std::ostream& operator<<(std::ostream& os, const value* val)
{
    if(val->is_array_)
    {
        os << '[';
    }

    switch (val->type_)
    {
        case value::data_type::none:
            os << "null";
            break;
        case value::data_type::integer:
            for(int i=0; i<val->len_; i++)
            {
                if(!val->is_bool_)
                {
                    os << val->int_[i];
                }
                else
                {
                    if(val->int_[i])
                    {
                        os << "true";
                    }
                    else
                    {
                        os << "false";
                    }
                }
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
            break;
        case value::data_type::real:
            for(int i=0; i<val->len_; i++)
            {
                os << val->real_[i];
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
            break;
        case value::data_type::string:
            for(int i=0; i<val->len_; i++)
            {
                os << '"' << val->str_[i] << '"';
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
            break;
        case value::data_type::object:
            for(int i=0; i<val->len_; i++)
            {
                os << val->conf_[i];
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
            break;
    }

    if(val->is_array_)
    {
        os << ']';
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const pair* pr)
{
    os << '"' << pr->name_ << '"' << ": ";
    os << pr->value_;
    return os;
}

std::ostream& operator<<(std::ostream& os, const config* conf)
{
    //jgb_debug("{ conf = %p }", conf);
    os << '{';
    if(conf)
    {
        int n = conf->pair_.size();
        int nn = 0;
        for (auto & i : conf->pair_) {
            os << i;
            ++ nn;
            if(nn < n)
            {
                os << ',';
            }
        }
    }
    os << '}';
    return os;
}

config::config()
{
}

config::~config()
{
    for (auto & i : pair_)
    {
        delete i;
    }
}

pair* config::find(const char* name)
{
    if(name)
    {
        for (auto it = pair_.begin(); it != pair_.end(); ++it)
        {
            if(!strcmp(name, (*it)->name_))
            {
                return *it;
            }
        }
    }
    return nullptr;
}

int config::add(const char* name, config* conf)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    pair* pr = find(name);
    if(!pr)
    {
        jgb::value* val = new jgb::value(jgb::value::data_type::object);
        val->conf_[0] = conf;
        pair_.push_back(new pair(name, val));
        return 0;
    }
    else
    {
        jgb_fail("config already exist. { name = %s }", name);
        return JGB_ERR_IGNORED;
    }
}

}
