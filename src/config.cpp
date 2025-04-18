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
#include "helper.h"

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
        delete [] int_;
    }
}

value::value(data_type type, int len, bool is_array, bool is_bool, pair *uplink)
    : uplink_(uplink)
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
        is_array_ = true;
    }
    else
    {
        is_array_ = is_array;
    }
    is_bool_ = is_bool;
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
        }
    }
    else
    {
        int_ = nullptr;
    }
}

int value::get(const char* path, value** val)
{
    if(!path || !val)
    {
        return JGB_ERR_INVALID;
    }

    //jgb_debug("get. { path = %s }", path);

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
        if(type_ == data_type::object)
        {
            // 注意：jpath_parse() 返回的 e 指向 ']' 的下一个字符。
            // 如果 s 是下标
            if(s < e && *s == '[' && *(e - 1) == ']')
            {
                int idx;
                r = str_to_index(idx, s, e - 1);
                if(!r && idx >= 0 && idx < len_)
                {
                    //jgb_debug("{ jpath = %s, idx = %d }", e, idx);
                    return conf_[idx]->get(e, val);
                }
            }
            else
            {
                //jgb_debug("{ jpath = %s }", s);
                return conf_[0]->get(s, val);
            }
        }

        jgb_debug("{ jpath = %s }", s);
        return JGB_ERR_INVALID;
    }
    else
    {
        *val = this;
        return 0;
    }
}

void value::get_path(std::string& path, int idx)
{
    if(uplink_)
    {
        uplink_->get_path(path);
    }
    else
    {
        path += "/";
    }
    if(idx && idx < len_)
    {
        path += '[' + std::to_string(idx) + ']';
    }
    //jgb_debug("{ path = %s }", path.c_str());
}

pair::pair(const char* name, value* value, config* uplink)
    : value_(value)
    , uplink_(uplink)
{
    // 疑问：对 name 和 value 区别对待，这么做合适吗？
    name_ = strdup(name);
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

config::config(value *uplink, int id)
    : uplink_(uplink),
      id_(id)
{
    jgb_assert(!pair_.size());
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

pair* config::find(const char* name, int n)
{
    //jgb_debug("find. { name = %.*s }", n, name);
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
                if(!strncmp(name, (*it)->name_, n))
                {
                    return *it;
                }
            }
        }
    }
    return nullptr;
}

int config::set(const char* name, int64_t ival, bool create, bool is_bool)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    pair* pr = find(name);
    if(!pr)
    {
        if(create)
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::integer, 1, false, is_bool);
            val->int_[0] = ival;
            val->valid_ = true;
            pair_.push_back(new pair(name, val, this));
            val->uplink_ = pair_.back();
            return 0;
        }
        return JGB_ERR_NOT_FOUND;
    }

    if(pr->value_->type_ == jgb::value::data_type::integer)
    {
        pr->value_->int_[0] = ival;
        pr->value_->valid_ = true;
        return 0;
    }
    else if(pr->value_->type_ == jgb::value::data_type::real)
    {
        pr->value_->real_[0] = ival;
        pr->value_->valid_ = true;
        return 0;
    }
    else
    {
        jgb_fail("set integer. { name = %s, type = %d }", name, (int) pr->value_->type_);
        return JGB_ERR_INVALID;
    }
}

int config::set(const char* name, double rval, bool create)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    pair* pr = find(name);
    if(!pr)
    {
        if(create)
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::real);
            val->real_[0] = rval;
            val->valid_ = true;
            pair_.push_back(new pair(name, val, this));
            val->uplink_ = pair_.back();
            return 0;
        }
        return JGB_ERR_NOT_FOUND;
    }

    if(pr->value_->type_ == jgb::value::data_type::real)
    {
        pr->value_->real_[0] = rval;
        pr->value_->valid_ = true;
        return 0;
    }
    else
    {
        jgb_fail("set real. { name = %s, type = %d }", name, (int) pr->value_->type_);
        return JGB_ERR_INVALID;
    }
}

// TODO: 把字符串值设置为 nullptr 或者 '/0'，valid_ 怎么算？
int config::set(const char* name, const char* sval, bool create)
{
    if(!name)
    {
        return JGB_ERR_INVALID;
    }

    pair* pr = find(name);
    if(!pr)
    {
        if(create)
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::string);
            if(sval)
            {
                val->str_[0] = strdup(sval);
            }
            // 字符串类型没有 unset 状态？
            val->valid_ = true;
            pair_.push_back(new pair(name, val, this));
            val->uplink_ = pair_.back();
            return 0;
        }
        return JGB_ERR_NOT_FOUND;
    }

    if(pr->value_->type_ == jgb::value::data_type::string)
    {
        if(pr->value_->str_[0])
        {
            free((void*)pr->value_->str_[0]);
        }
        if(sval)
        {
            pr->value_->str_[0] = strdup(sval);
        }
        jgb_assert(pr->value_->valid_);
        return 0;
    }
    else
    {
        jgb_fail("type mismatch. { name = %s, type = %d }", name, (int) pr->value_->type_);
        return JGB_ERR_INVALID;
    }
}

int config::set(const char* name, const std::string& sval, bool create)
{
    return set(name, sval.c_str(), create);
}

int config::create(const char* name, config* cval)
{
    if(!name || !cval)
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
    if(!name)
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
    if(!name)
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

int config::get(const char* path, value** val)
{
    if(!path || !val)
    {
        return JGB_ERR_INVALID;
    }

    int r;
    const char* s = path;
    const char* e;

    r = jpath_parse(&s, &e);
    if(!r)
    {
        if(*s != '\0')
        {
            pair* pr = find(s, (int)(e - s));
            if(pr)
            {
                return pr->value_->get(e, val);
            }
        }
    }

    *val = nullptr;
    //jgb_debug("not found. { s = %.*s }", (int)(e - s), s);
    return JGB_ERR_NOT_FOUND;
}

int config::get(const char* path, value** val, int& idx)
{
    if(path && val)
    {
        int r;
        std::string base;

        r = get_base_index(path, base, idx);
        if(!r)
        {
            //jgb_debug("{ path = %s, base = %s, idx = %d}", path, base.c_str(), idx);
            return get(base.c_str(), val);
        }
    }
    return JGB_ERR_INVALID;
}

int config::get(const char* path, int& ival)
{
    int r;
    int idx;
    value* pval;

    r = get(path, &pval, idx);
    if(!r)
    {
        jgb_assert(pval);
        if(pval->type_ == value::data_type::integer
                && pval->valid_
                && pval->len_ > idx)
        {
            ival = pval->int_[idx];
            return 0;
        }
    }
    return JGB_ERR_FAIL;
}

int config::get(const char* path, double& rval)
{
    int r;
    int idx;
    value* pval;

    r = get(path, &pval, idx);
    if(!r)
    {
        jgb_assert(pval);
        if(pval->type_ == value::data_type::real
                && pval->valid_
                && pval->len_ > idx)
        {
            rval = pval->real_[idx];
            return 0;
        }
    }
    return JGB_ERR_FAIL;
}

int config::get(const char* path, const char** sval)
{
    int r;
    int idx;
    value* pval;

    r = get(path, &pval, idx);
    if(!r)
    {
        jgb_assert(pval);
        if(pval->type_ == value::data_type::string
                && pval->valid_
                && pval->len_ > idx)
        {
            *sval = pval->str_[idx];
            return 0;
        }
    }
    return JGB_ERR_FAIL;
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

    r = get(path, &pval, idx);
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
                if(dest->is_bool_ == src->is_bool_)
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
                            jgb_debug("diff. { path = %s, dest = %ld, src = %ld }",
                                      path.c_str(), dest->int_[i], src->int_[i]);
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
                              "dest.type_ = %d, dest.is_bool_ = %d, " \
                              "src.type_ = %d, src.is_bool = %d }",
                              path.c_str(),
                              (int) dest->type_, dest->is_bool_,
                              (int) src->type_, src->is_bool_);
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
                        jgb_debug("diff. { path = %s, dest = %f, src = %f }",
                                  path.c_str(), dest->real_[i], src->real_[i]);
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
                            jgb_debug("diff. { path = %s, dest = %s, src = %s }",
                                      path.c_str(), dest->str_[i], src->str_[i]);
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
                    jgb_debug("diff. { path = %s, dest = %f, src = %ld }",
                              path.c_str(), dest->real_[i], src->int_[i]);
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

}
