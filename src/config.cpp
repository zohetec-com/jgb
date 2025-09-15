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
#include "log.h"
#include "helper.h"
#include "constrains.h"

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
                if(str_[i])
                {
                    free((void*)str_[i]);
                }
            }
        }
        else if(type_ == data_type::object)
        {
            for(int i=0; i<len_; i++)
            {
                if(conf_[i])
                {
                    delete conf_[i];
                }
            }
        }
        if(!binded_)
        {
            delete [] int_;
        }
    }
}

int value::bind(void* val)
{
    if(type_ == data_type::integer || type_ == data_type::real)
    {
        if(!binded_)
        {
            delete [] int_;
        }
        int_ = static_cast<int64_t*>(val);
        binded_ = true;
        return 0;
    }
    else
    {
        return JGB_ERR_NOT_SUPPORT;
    }
}

value::value(const value& other)
{
    len_ = other.len_;
    type_ = other.type_;
    array_ = other.array_;
    bool_ = other.bool_;
    valid_ = other.valid_;
    // 调用者(caller)应当初始化 uplink_。
    uplink_ = nullptr;
    int_ = len_ > 0 ? new int64_t[len_]{} : nullptr;
    if(valid_)
    {
        jgb_assert(len_ > 0);
        if(type_ == data_type::string)
        {
            for(int i=0; i<len_; i++)
            {
                if(other.str_[i])
                {
                    str_[i] = strdup(other.str_[i]);
                }
                else
                {
                    str_[i] = nullptr;
                }
            }
        }
        else if(type_ == data_type::object)
        {
            for(int i=0; i<len_; i++)
            {
                if(other.conf_[i])
                {
                    conf_[i] = new config(*other.conf_[i]);
                }
                else
                {
                    conf_[i] = nullptr;
                }
            }
        }
        else if(type_ == data_type::integer || type_ == data_type::real)
        {
            jgb_assert(sizeof(int64_t) == sizeof(double));
            for(int i=0; i<len_; i++)
            {
                int_[i] = other.int_[i];
            }
        }
        else
        {
            jgb_assert(0);
        }
    }
}

void swap(value& a, value& b)
{
    std::swap(a.type_, b.type_);
    std::swap(a.len_, b.len_);
    std::swap(a.array_, b.array_);
    std::swap(a.bool_, b.bool_);
    std::swap(a.valid_, b.valid_);
    std::swap(a.int_, b.int_);
    std::swap(a.binded_, b.binded_);
    std::swap(a.uplink_, b.uplink_);
}

value& value::operator=(value val)
{
    swap(*this, val);
    return *this;
}

value::value(data_type type, int len, bool is_array, bool is_bool, pair *uplink)
    : binded_(false),
      uplink_(uplink)
{
    if(len > object_len_max)
    {
        jgb_warning("请求创建数组，长度超限，已截断处理！{ len = %d，object_len_max = %d }", len, object_len_max);
        len = object_len_max;
    }

    type_ = type;
    len_ = len;
    if(len > 1)
    {
        array_ = true;
    }
    else
    {
        array_ = is_array;
    }
    bool_ = is_bool;
    valid_ = false;

    if(len_ > 0)
    {
        // https://www.reddit.com/r/cpp_questions/comments/mgmuuu/when_allocating_an_array_with_new_in_c_does_it/
        int_ = new int64_t[len]{};
        jgb_assert(int_);
        if(type_ == data_type::object)
        {
            for(int i=0; i<len_; i++)
            {
                conf_[i] = new config;
            }
            valid_ = true;
        }
        else if(type_ == data_type::string)
        {
            valid_ = true;
        }
    }
    else
    {
        int_ = nullptr;
    }
}

int value::get(const char* path, value** val, int* idx)
{
    if(!path || !val)
    {
        return JGB_ERR_INVALID;
    }

    //jgb_debug("{ path = %s }", path);

    int r;
    const char* s = path;
    const char* e;

    r = jpath_parse(&s, &e);
    if(r)
    {
        jgb_debug("jpath_parse failed. { path = %s }", path);
        return r;
    }

    //jgb_debug("{ s = %.*s, type_ = %d, e = %c }", (int)(e - s), s, (int)type_, *e);

    if(*s != '\0')
    {
        int xidx;
        r = str_to_index(xidx, s, e - 1);
        if(r)
        {
            if(type_ == data_type::object)
            {
                return conf_[0]->get(s, val, idx);
            }
            // invalid
        }
        else
        {
            if(xidx >= 0 && xidx < len_)
            {
                if(type_ == data_type::object)
                {
                    if(*e != '\0')
                    {
                        return conf_[xidx]->get(e, val, idx);
                    }
                }
                if(*e == '\0')
                {
                    *val = this;
                    if(idx)
                    {
                        *idx = xidx;
                    }
                    return 0;
                }
                jgb_debug("{ e = %s }", e);
                // invalid
            }
            else
            {
                //jgb_debug("not found. { s = %s }", s);
                return JGB_ERR_NOT_FOUND;
            }
        }

        jgb_debug("invalid. { s = %s }", s);
        return JGB_ERR_INVALID;
    }
    else
    {
        *val = this;
        if(idx)
        {
            *idx = 0;
        }
        return 0;
    }
}

void value::get_path(std::string& path, int idx, bool show_idx_0)
{
    if(uplink_)
    {
        uplink_->get_path(path);
    }
    else
    {
        path += "/";
    }
    //jgb_debug("idx = %d, show_idx_0 = %d, final = %d", idx, show_idx_0, !schema && (idx || show_idx_0) && idx < len_);
    if((idx || (array_ && show_idx_0)) && idx < len_)
    {
        path += '[' + std::to_string(idx) + ']';
    }
    //jgb_debug("{ path = %s }", path.c_str());
}

int64_t value::int64(int idx, int64_t def)
{
    int64_t lval = def;
    get(lval, idx);
    return lval;
}

std::string value::str(int idx, const std::string def)
{
    std::string sval = def;
    get(sval, idx);
    return sval;
}

double value::real(int idx, double def)
{
    double rval = def;
    get(rval, idx);
    return rval;
}

int value::get(bool& bval, int idx)
{
    int64_t lval;
    int r = get(lval, idx);
    if(!r)
    {
        bval = (lval != 0);
    }
    return r;
}

int value::get(int16_t& sival, int idx)
{
    int64_t lval;
    int r = get(lval, idx);
    if(!r)
    {
        sival = lval;
    }
    return r;
}

int value::get(int& ival, int idx)
{
    int64_t lval;
    int r = get(lval, idx);
    if(!r)
    {
        ival = lval;
    }
    return r;
}

int value::get(int64_t& lval, int idx)
{
    if(valid_
        && idx < len_
        && idx >= 0)
    {
        if(type_ == data_type::integer)
        {
            lval = int_[idx];
            return 0;
        }
        if(type_ == data_type::real)
        {
            lval = real_[idx];
            return 0;
        }
        if(type_ == data_type::string)
        {
            int r;
            r = stoll(str_[idx], lval);
            if(!r)
            {
                return 0;
            }
            if(!strcasecmp("true", str_[idx]))
            {
                lval = 1L;
                return 0;
            }
            if(!strcasecmp("false", str_[idx]))
            {
                lval = 0L;
                return 0;
            }
        }
    }
    return JGB_ERR_INVALID;
}

int value::get(double& rval, int idx)
{
    if(valid_
        && idx < len_
        && idx >= 0)
    {
        if(type_ == data_type::real)
        {
            rval = real_[idx];
            return 0;
        }
        if(type_ == data_type::integer)
        {
            rval = int_[idx];
            return 0;
        }
        if(type_ == data_type::string)
        {
            return stod(str_[idx], rval);
        }
    }
    return JGB_ERR_INVALID;
}

int value::get(const char** sval, int idx)
{
    if( sval
        && idx < len_
        && idx >= 0)
    {
        if(type_ == data_type::string)
        {
            *sval = str_[idx];
            return 0;
        }
    }
    return JGB_ERR_INVALID;
}

int value::get(std::string& sval, int idx)
{
    int r;
    const char* s;
    r = get(&s, idx);
    if(!r)
    {
        sval = std::string(s);
    }
    return r;
}

int value::set(bool bval, int idx)
{
    return set(static_cast<int64_t>(bval), idx);
}

int value::set(int ival, int idx)
{
    return set(static_cast<int64_t>(ival), idx);
}

int value::set(int64_t lval, int idx)
{
    if(idx < len_
        && idx >= 0)
    {
        if(type_ == value::data_type::integer)
        {
            int_[idx] = lval;
            valid_ = true;
            return 0;
        }
        else if(type_ == value::data_type::real)
        {
            real_[idx] = lval;
            valid_ = true;
            return 0;
        }
    }
    return JGB_ERR_INVALID;
}

int value::set(double rval, int idx)
{
    if(idx < len_
        && idx >= 0)
    {
        if(type_ == jgb::value::data_type::real)
        {
            real_[idx] = rval;
            valid_ = true;
            return 0;
        }
    }
    return JGB_ERR_INVALID;
}

int value::set(const char* sval, int idx)
{
    if(idx < len_
        && idx >= 0)
    {
        if(type_ == jgb::value::data_type::string)
        {
            if(str_[idx])
            {
                free((void*)str_[idx]);
                str_[idx] = nullptr;
            }
            if(sval)
            {
                str_[idx] = strdup(sval);
            }
            jgb_assert(valid_);
            return 0;
        }
        // 如果 json 转为 config/value 时，字符串类型的值已转换为 int 或 real 类型，
        // 应该没有必要支持以字符串类型设置 int, real 类型了吧。
    }
    return JGB_ERR_INVALID;
}

int value::set(const std::string& sval, int idx)
{
    return set(sval.c_str(), idx);
}

pair::pair(const char* name, value* value, config* uplink)
    : value_(value)
    , uplink_(uplink)
{
    // 疑问：对 name 和 value 区别对待，这么做合适吗？
    name_ = strdup(name);
}

pair::pair(const pair& other)
{
    name_ = strdup(other.name_);
    value_ = new value(*other.value_);
    value_->uplink_ = this;
}

void swap(pair& a, pair& b)
{
    std::swap(a.name_,b.name_);
    std::swap(a.value_,b.value_);
    std::swap(a.uplink_,b.uplink_);
}

pair& pair::operator=(pair p)
{
    swap(*this, p);
    return *this;
}

pair::~pair()
{
    free((void*) name_);
    delete value_;
}

void pair::get_path(std::string& path)
{
    if(uplink_)
    {
        uplink_->get_path(path);
    }
    else
    {
        jgb_assert(0);
    }
    if(path.back() != '/')
    {
        path += '/';
    }
    path += std::string(name_);
    //jgb_debug("{ name_ = %s, path = %s }", name_, path.c_str());
}

std::ostream& operator<<(std::ostream& os, const value* val)
{
    if(val->array_)
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
                if(!val->bool_)
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

    if(val->array_)
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

config::config(value *uplink, int id)
    : uplink_(uplink),
      id_(id)
{
    jgb_assert(!pair_.size());
}

config::config(const config& other)
{
    id_ = other.id_;
    for (auto & i : other.pair_)
    {
        pair_.push_back(new pair(*i));
        pair_.back()->uplink_ = this;
    }
}

void swap(config& a, config& b)
{
    std::swap(a.id_, b.id_);
    std::swap(a.uplink_, b.uplink_);
    std::swap(a.pair_, b.pair_);
}

config& config::operator=(config c)
{
    swap(*this, c);
    return *this;
}

config::~config()
{
    clear();
}

void config::clear()
{
    for (auto & i : pair_)
    {
        delete i;
    }
    pair_.clear();
}

pair* config::find(const char* name, int n) const
{
    //jgb_debug("find. { name = %.*s, n = %d }", n, name, n);
    if(name)
    {
        for (auto it = pair_.begin(); it != pair_.end(); ++it)
        {
            if(!n)
            {
                if(!strcmp(name, (*it)->name_))
                {
                    return *it;
                }
            }
            else
            {
                if(!strncmp(name, (*it)->name_, n) && n == (int) strlen((*it)->name_))
                {
                    return *it;
                }
            }
        }
    }
    return nullptr;
}

int config::set(const char* path, bool bval)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->set(bval, idx);
    }
#ifdef DEBUG
    jgb_fail("set { r = %d, path = %s, bval = %d }", r, path, bval);
#endif
    return r;
}

int config::set(const char* path, int ival)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->set(ival, idx);
    }
#ifdef DEBUG
    jgb_fail("set { r = %d, path = %s, ival = %d }", r, path, ival);
#endif
    return r;
}

int config::set(const char* path, int64_t lval)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->set(lval, idx);
    }
#ifdef DEBUG
    jgb_fail("set { r = %d, path = %s, lval = %ld }", r, path, lval);
#endif
    return r;
}

int config::set(const char* path, double rval)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->set(rval, idx);
    }
#ifdef DEBUG
    jgb_fail("set { r = %d, path = %s, rval = %f }", r, path, rval);
#endif
    return r;
}

int config::set(const char* path, const char* sval)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->set(sval, idx);
    }
#ifdef DEBUG
    jgb_fail("set { r = %d, path = %s, sval = %s }", r, path, sval ? sval : "null");
#endif
    return r;
}

int config::set(const char* path, const std::string& sval)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->set(sval, idx);
    }
#ifdef DEBUG
    jgb_fail("set { r = %d, path = %s, sval = %s }", r, path, sval.c_str());
#endif
    return r;
}

#define make_path(v) \
char path[JGB_JPATH_MAX_LEN]; \
    do { \
        va_list args; \
        int r; \
        va_start(args, v); \
        r = vsnprintf(path, JGB_JPATH_MAX_LEN, format, args); \
        if(r < 0) \
        { \
                jgb_warning("vsnprintf failed. { format = %s }", format); \
                va_end(args); \
                return JGB_ERR_FAIL; \
        } \
        if(r >= JGB_JPATH_MAX_LEN) \
        { \
                jgb_warning("format too long. { format = %s }", format); \
                va_end(args); \
                return JGB_ERR_LIMIT; \
        } \
        /* jgb_debug("{ path = %s }", path); */ \
        va_end(args); \
    } while (0)

#define make_path_def(def) \
char path[JGB_JPATH_MAX_LEN]; \
    do { \
        va_list args; \
        int r; \
        va_start(args, def); \
        r = vsnprintf(path, JGB_JPATH_MAX_LEN, format, args); \
        if(r < 0) \
        { \
            jgb_warning("vsnprintf failed. { format = %s }", format); \
            va_end(args); \
            return def; \
        } \
        if(r >= JGB_JPATH_MAX_LEN) \
        { \
            jgb_warning("format too long. { format = %s }", format); \
            va_end(args); \
            return def; \
        } \
        /* jgb_debug("{ path = %s }", path); */ \
        va_end(args); \
} while (0)

int config::setf(const char* format, bool bval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(bval);
    return set(path, bval);
}

int config::setf(const char* format, int ival, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(ival);
    return set(path, ival);
}

int config::setf(const char* format, int64_t lval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(lval);
    return set(path, lval);
}

int config::setf(const char* format, double rval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(rval);
    return set(path, rval);
}

int config::setf(const char* format, const char* sval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(sval);
    return set(path, sval);
}

int config::setf(const char* format, const std::string& sval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(sval);
    return set(path, sval);
}

int config::bind(const char* path, void* val)
{
    int r;
    value* pval;
    r = get(path, &pval);
    if(!r)
    {
        jgb_assert(pval);
        return pval->bind(val);
    }
#ifdef DEBUG
    jgb_fail("bind { path = %s }", path);
#endif
    return r;
}

int config::bindf(const char* format, void* val, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(val);
    return bind(path, val);
}

int config::create(const char* name, bool bval)
{
    return create(name, static_cast<int64_t>(bval), true);
}

int config::create(const char* name, int ival, bool is_bool)
{
    return create(name, static_cast<int64_t>(ival), is_bool);
}

int config::create(const char* name, int64_t lval, bool is_bool)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }
    pair* pr = find(name);
    if(!pr)
    {
        jgb::value* val = new jgb::value(jgb::value::data_type::integer, 1, false, is_bool);
        val->int_[0] = lval;
        val->valid_ = true;
        pair_.push_back(new pair(name, val, this));
        val->uplink_ = pair_.back();
        return 0;
    }
    return JGB_ERR_IGNORED;
}

int config::create(const char* name, double rval)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }
    pair* pr = find(name);
    if(!pr)
    {
        jgb::value* val = new jgb::value(jgb::value::data_type::real, 1, false);
        val->real_[0] = rval;
        val->valid_ = true;
        pair_.push_back(new pair(name, val, this));
        val->uplink_ = pair_.back();
        return 0;
    }
    return JGB_ERR_IGNORED;
}

int config::create(const char* name, const char* sval)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }
    pair* pr = find(name);
    if(!pr)
    {
        jgb::value* val = new jgb::value(jgb::value::data_type::string, 1, false);
        jgb_assert(!val->str_[0]);
        if(sval)
        {
            val->str_[0] = strdup(sval);
        }
        jgb_assert(val->valid_);
        pair_.push_back(new pair(name, val, this));
        val->uplink_ = pair_.back();
        return 0;
    }
    return JGB_ERR_IGNORED;
}

int config::create(const char* name, const std::string& sval)
{
    return create(name, sval.c_str());
}

int config::create(const char* name, config* cval)
{
    if(!name || strchr(name, '/') || !cval)
    {
        return JGB_ERR_INVALID;
    }
    pair* pr = find(name);
    if(!pr)
    {
        jgb::value* val = new jgb::value(jgb::value::data_type::object);
        cval->uplink_ = val;
        jgb_assert(val->conf_[0]);
        delete val->conf_[0];
        val->conf_[0] = cval;
        val->valid_ = true;
        pair_.push_back(new pair(name, val, this));
        val->uplink_ = pair_.back();
        return 0;
    }
    jgb_fail("config already exist. { name = %s }", name);
    return JGB_ERR_IGNORED;
}

int config::create(const char* name)
{
    if(!name || strchr(name, '/'))
    {
        return JGB_ERR_INVALID;
    }
    pair* pr = find(name);
    if(!pr)
    {
        jgb::value* val = new jgb::value(jgb::value::data_type::none);
        pair_.push_back(new pair(name, val, this));
        val->uplink_ = pair_.back();
        return 0;
    }
    return JGB_ERR_IGNORED;
}

int config::create(const char* name, value* val)
{
    if(!name || strchr(name, '/'))
    {
        return JGB_ERR_INVALID;
    }
    pair* pr = find(name);
    if(!pr)
    {
        pair_.push_back(new pair(name, val, this));
        val->uplink_ = pair_.back();
        return 0;
    }
    return JGB_ERR_IGNORED;
}

int config::remove(const char* name)
{
    if(!name || strchr(name, '/'))
    {
        return JGB_ERR_INVALID;
    }
    for(std::list<pair*>::iterator it = pair_.begin(); it != pair_.end(); ++it)
    {
        if(!strcmp((*it)->name_, name))
        {
            delete (*it);
            pair_.erase(it);
            return 0;
        }
    }
    return JGB_ERR_IGNORED;
}

int config::get(const char* path, value** val, int* idx)
{
    if(!path || !val)
    {
        return JGB_ERR_INVALID;
    }

    //jgb_debug("{ path = %s }", path);

    int r;
    const char* s = path;
    const char* e;

    r = jpath_parse(&s, &e);
    if(!r)
    {
        if(*s != '\0')
        {
            int xidx;
            r = str_to_index(xidx, s, e);
            //jgb_debug("{ r = %d, s = %.*s }", r, (int)(e - s), s);
            if(r)
            {
                pair* pr = find(s, (int)(e - s));
                if(pr)
                {
                    return pr->value_->get(e, val, idx);
                }
                else
                {
                    return JGB_ERR_NOT_FOUND;
                }
            }
        }
    }

    *val = nullptr;
    jgb_debug("invalid. { path = %s, s = %.*s }", path, (int)(e - s), s);
    return JGB_ERR_INVALID;
}

int config::get(const char* path, bool& bval)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->get(bval, idx);
    }
    return r;
}

int config::get(const char* path, int16_t& sival)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->get(sival, idx);
    }
    return r;
}

int config::get(const char* path, int& ival)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->get(ival, idx);
    }
    return r;
}

int config::get(const char* path, int64_t& lval)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->get(lval, idx);
    }
    return r;
}

int config::getf(const char* format, value** val, ...)
{
    if(!format || !val)
    {
        return JGB_ERR_INVALID;
    }
    make_path(val);
    return get(path, val);
}

int config::getf(const char* format, bool& bval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(bval);
    return get(path, bval);
}

int config::getf(const char* format, int& ival, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(ival);
    return get(path, ival);
}

int config::getf(const char* format, int64_t& lval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(lval);
    return get(path, lval);
}

int config::getf(const char* format, double& rval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(rval);
    return get(path, rval);
}

int config::getf(const char* format, config** cval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(cval);
    return get(path, cval);
}


int config::getf(const char* format, const char** sval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(sval);
    return get(path, sval);
}

int config::getf(const char* format, std::string& sval, ...)
{
    if(!format)
    {
        return JGB_ERR_INVALID;
    }
    make_path(sval);
    return get(path, sval);
}

int64_t config::int64(const char* path, int64_t def)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->int64(idx, def);
    }
    return def;
}

std::string config::str(const char* path, const std::string def)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->str(idx, def);
    }
    return def;
}

double config::real(const char* path, double def)
{
    int r;
    int idx;
    value* pval;
    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->real(idx, def);
    }
    return def;
}

int64_t config::int64f(const char* format, int64_t def, ...)
{
    if(!format)
    {
        return def;
    }
    make_path_def(def);
    return int64(path, def);
}

std::string config::strf(const char* format, const std::string def, ...)
{
    if(!format)
    {
        return def;
    }
    make_path_def(def);
    return str(path, def);
}

double config::realf(const char* format, double def, ...)
{
    if(!format)
    {
        return def;
    }
    make_path_def(def);
    return real(path, def);
}

int config::get(const char* path, double& rval)
{
    int r;
    int idx;
    value* pval;

    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->get(rval, idx);
    }
    return JGB_ERR_FAIL;
}

int config::get(const char* path, const char** sval)
{
    int r;
    int idx;
    value* pval;

    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        return pval->get(sval, idx);
    }
    return r;
}

int config::get(const char* path, std::string& sval)
{
    int r;
    const char* s;

    r = get(path, &s);
    if(!r)
    {
        sval = std::string(s);
    }
    return r;
}

int config::get(const char* path, config** cval)
{
    int r;
    int idx;
    value* pval;

    r = get(path, &pval, &idx);
    if(!r)
    {
        jgb_assert(pval);
        if(pval->type_ == value::data_type::object
                && pval->valid_
                && pval->len_ > idx)
        {
            *cval = pval->conf_[idx];
            return 0;
        }
    }
    return JGB_ERR_FAIL;
}

std::string config::to_string()
{
    std::ostringstream oss;
    oss << this;
    return oss.str();
}

void config::get_path(std::string& path)
{
    //jgb_debug("{ uplink_ = %p }", uplink_);
    if(uplink_)
    {
        uplink_->get_path(path);
        if(id_)
        {
            path += '[' + std::to_string(id_) + ']';
        }
    }
    else
    {
        path += "/";
        jgb_assert(!id_);
    }
    //jgb_debug("{ path = %s }", path.c_str());
}

int update(config* dest, config* src, std::list<std::string> *diff, bool dry_run)
{
    if(!dest || ! src)
    {
        return JGB_ERR_INVALID;
    }

    pair* pr;
    for(auto i: src->pair_)
    {
        pr = dest->find(i->name_);
        if(pr)
        {
            update(pr->value_, i->value_, diff, dry_run);
        }
    }
    return 0;
}

int update(value* dest, value* src, std::list<std::string>* diff, bool dry_run)
{
    if(!dest || !src)
    {
        return JGB_ERR_INVALID;
    }

    if(dest->len_ == src->len_)
    {
        if(dest->type_ == src->type_)
        {
            if(dest->type_ == value::data_type::integer)
            {
                if(dest->bool_ == src->bool_)
                {
                    // FIXME! valid_ 需要考虑吗？
                    for(int i=0; i<dest->len_; i++)
                    {
                        if(dest->int_[i] != src->int_[i])
                        {
                            std::string path;
                            src->get_path(path, i);
                            if(diff)
                            {
                                diff->push_back(path);
                            }
                            //jgb_debug("diff. { path = %s, dest = %ld, src = %ld }",
                            //          path.c_str(), dest->int_[i], src->int_[i]);
                            if(!dry_run)
                            {
                                dest->int_[i] = src->int_[i];
                            }
                        }
                    }
                }
                else
                {
                    std::string path;
                    src->get_path(path);
                    jgb_debug("type unmatched. { path = %s, " \
                              "dest.type_ = %d, dest.bool_ = %d, " \
                              "src.type_ = %d, src.is_bool = %d }",
                              path.c_str(),
                              (int) dest->type_, dest->bool_,
                              (int) src->type_, src->bool_);
                }
            }
            else if(dest->type_ == value::data_type::real)
            {
                // FIXME! valid_
                for(int i=0; i<dest->len_; i++)
                {
                    if(!is_equal(dest->real_[i], src->real_[i]))
                    {
                        std::string path;
                        src->get_path(path, i);
                        if(diff)
                        {
                            diff->push_back(path);
                        }
                        //jgb_debug("diff. { path = %s, dest = %f, src = %f }",
                        //          path.c_str(), dest->real_[i], src->real_[i]);
                        if(!dry_run)
                        {
                            dest->real_[i] = src->real_[i];
                        }
                    }
                }
            }
            else if(dest->type_ == value::data_type::string)
            {
                for(int i=0; i<dest->len_; i++)
                {
                    if(dest->str_[i] && src->str_[i])
                    {
                        if( (!dest->str_[i] && src->str_[i])
                            || (dest->str_[i] && !src->str_[i])
                            || (dest->str_[i] && src->str_[i] && strcmp(dest->str_[i], src->str_[i])))
                        {
                            std::string path;
                            src->get_path(path, i);
                            if(diff)
                            {
                                diff->push_back(path);
                            }
                            //jgb_debug("diff. { path = %s, dest = %s, src = %s }",
                            //          path.c_str(), dest->str_[i], src->str_[i]);
                            if(!dry_run)
                            {
                                if(dest->str_[i])
                                {
                                    free((void*)dest->str_[i]);
                                    dest->str_[i] = nullptr;
                                }
                                if(src->str_[i])
                                {
                                    dest->str_[i] = strdup(src->str_[i]);
                                }
                            }
                        }
                    }
                }
            }
            else if(dest->type_ == value::data_type::object)
            {
                for(int i=0; i<dest->len_; i++)
                {
                    update(dest->conf_[i], src->conf_[i], diff, dry_run);
                }
            }
            else
            {
                std::string path;
                src->get_path(path);
                jgb_warning("ignored. { path = %s, type = %d }", path.c_str(), (int)dest->type_);
            }
        }
        else if(dest->type_ == value::data_type::real && src->type_ == value::data_type::integer)
        {
            // FIXME! valid_
            for(int i=0; i<dest->len_; i++)
            {
                if(!is_equal(dest->real_[i], src->int_[i]))
                {
                    std::string path;
                    src->get_path(path, i);
                    if(diff)
                    {
                        diff->push_back(path);
                    }
                    //jgb_debug("diff. { path = %s, dest = %f, src = %ld }",
                    //          path.c_str(), dest->real_[i], src->int_[i]);
                    if(!dry_run)
                    {
                        dest->real_[i] = src->int_[i];
                    }
                }
            }
        }
        else
        {
            std::string path;
            src->get_path(path);
            jgb_debug("type unmatched. { path = %s, dest.type_ = %d, src.type_ = %d }",
                      path.c_str(), (int) dest->type_, (int) src->type_);
        }
    }
    else
    {
        std::string path;
        src->get_path(path);
        jgb_debug("size unmatched. { path = %s, dest.len_ = %d, src.len_ = %d }",
                  path.c_str(), dest->len_, src->len_);
    }
    return 0;
}

void find(value* v, const std::string& name, value::data_type type, void(*on_found)(value*,void*), void* arg)
{
    if(on_found)
    {
        if(v->type_ == value::data_type::object)
        {
            for(int i=0; i<v->len_; i++)
            {
                find(v->conf_[i], name, type, on_found, arg);
            }
        }
    }
}

void find(config* c, const std::string& name, value::data_type type, void(*on_found)(value*,void*), void* arg)
{
    if(on_found)
    {
        for(auto i: c->pair_)
        {
            if(!strcmp(name.c_str(), i->name_) && i->value_->type_ == type)
            {
                on_found(i->value_, arg);
            }
            if(i->value_->type_ == value::data_type::object)
            {
                find(i->value_, name, type, on_found, arg);
            }
        }
    }
}

}
