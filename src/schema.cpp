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
#include "schema.h"
#include "helper.h"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace jgb
{

static int scan_one_interval(const char* str, jgb::value::data_type type, jgb::range::interval &interval);
static bool validate(int64_t ival, jgb::range::interval& interval);

range::range(value::data_type type, value* v_size, bool is_required, bool is_array)
    : type_(type),
    len_(1),
    inter_len_(nullptr),
    is_required_(is_required),
    is_array_(is_array)
{
    // 参数的长度：
    // 1. 缺省为 1。
    // 2. 可以设置为 0，表示无限制。
    // 3. 也可以使用一个大于 0 的整数来限定参数的长度。
    // 4. 也可以使用一个取值区间来限制参数的长度的范围。
    if(!v_size)
    {
        len_ = 1;
    }
    else
    {
        if(v_size->type_ == value::data_type::integer)
        {
            if(v_size->len_ > 0)
            {
                len_ = v_size->int_[0];
                if(len_ < 0)
                {
                    jgb_warning("invalid size. reset to 0. { size = %d }", len_);
                    len_ = 0;
                }
            }
        }
        else if(v_size->type_ == value::data_type::string)
        {
            if(v_size->len_ > 0 && v_size->str_[0])
            {
                len_ = 0;
                inter_len_ = new struct interval;
                int r = scan_one_interval(v_size->str_[0], jgb::value::data_type::integer, *inter_len_);
                if(r)
                {
                    jgb_warning("invalid interval. { text = %s }", v_size->str_[0]);

                    delete inter_len_;
                    inter_len_ = nullptr;
                }
            }
        }
        else
        {
            jgb_warning("invalid size type. reset to 0. { type = %d }", v_size->type_);
            len_ = 0;
        }
    }
    if(len_ != 1)
    {
        is_array_ = true;
    }
}

range::~range()
{
    delete inter_len_;
}

static void put_result(value* val, int idx, schema::result *res, int code, bool show_idx_0 = true)
{
    jgb_assert(val);
    //jgb_debug("res = %p", res);
    if(res)
    {
        std::string path;
        val->get_path(path, idx, show_idx_0);
        //jgb_debug("{ code = %d, path = %s }", code, path.c_str());
        if(!code)
        {
            res->ok_.push_back(path);
        }
        else
        {
            res->error_.push_back({path, code});
        }
    }
}

int range::validate_type(value* val, schema::result *res)
{
    if(val->type_ != type_)
    {
        jgb_debug("{ val->type_ = %d, type_ = %d }", val->type_, type_);
        if(val->type_ == value::data_type::integer && type_ == value::data_type::real)
        {
            // 可以将整数类型赋值给实数类型，但反过来不可以。
            // 理由：可能损失精度。
        }
        else if(val->type_ == value::data_type::string)
        {
            // 字符串类型可能可以转换为 int, real。
        }
        else
        {
            put_result(val, 0, res, JGB_ERR_SCHEMA_NOT_MATCHED_TYPE, false);
            return JGB_ERR_SCHEMA_NOT_MATCHED_TYPE;
        }
    }
    return 0;
}

int range::validate(value* val, schema::result* res)
{
    if(!val)
    {
        return JGB_ERR_INVALID;
    }

    if(len_ || val->len_)
    {
        int r = validate_type(val, res);
        if(r)
        {
            return r;
        }
    }

    //jgb_debug("{ val->len_ = %d, len_ = %d }", val->len_, len_);

    if(len_)
    {
        if(val->len_ != len_)
        {
            put_result(val, 0, res, JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH, false);
            return JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH;
        }
    }
    else
    {
        if(inter_len_)
        {
            if(!jgb::validate(val->len_, *inter_len_))
            {
                put_result(val, 0, res, JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH, false);
                return JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH;
            }
        }
    }

    return 0;
}

range_enum::range_enum(value *v_size, bool is_required, bool is_array, value* range_val_)
    : range(value::data_type::integer, v_size, is_required, is_array)
{
    jgb_assert(range_val_);
    jgb_assert(range_val_->type_ == value::data_type::integer);
    jgb_assert(range_val_->len_ > 0);
    enum_int_.resize(range_val_->len_);
    for(int i = 0; i < range_val_->len_; i++)
    {
        enum_int_[i] = range_val_->int_[i];
    }
    jgb_assert(range_val_->uplink_);
    jgb_assert(range_val_->uplink_->uplink_);
    config* c = range_val_->uplink_->uplink_;
    value* alias_val;
    int r;
    r = c->get("alias", &alias_val);
    if(!r)
    {
        if(alias_val->type_ == value::data_type::string)
        {
            if(alias_val->len_ == range_val_->len_)
            {
                enum_name_.resize(alias_val->len_);
                for(int i=0; i<alias_val->len_; i++)
                {
                    enum_name_[i] = std::string(alias_val->str_[i]);
                }
            }
            else
            {
                jgb_warning("invalid alias size.");
            }
        }
        else
        {
            jgb_warning("invalid alias type");
        }
    }
}

range_enum::range_enum(value *v_size, bool is_required, bool is_array)
    : range(value::data_type::integer, v_size, is_required, is_array)
{
    enum_int_.resize(2);
    enum_int_[0] = 0;
    enum_int_[0] = 1;
    jgb_assert(enum_name_.empty());
}

int range_enum::validate(const char* str)
{
    for(auto i: enum_name_)
    {
        if(!strcasecmp(str, i.c_str()))
        {
            return 0;
        }
    }
    return JGB_ERR_SCHEMA_NOT_MATCHED_RANGE;
}

int range_enum::validate(int ival)
{
    for(uint i=0; i<enum_int_.size(); i++)
    {
        if(ival == enum_int_[i])
        {
            //jgb_debug("valid int. { ival = %ld }", ival);
            return 0;
        }
    }
    return JGB_ERR_SCHEMA_NOT_MATCHED_RANGE;
}

int range_enum::validate_type(value* val, schema::result *res)
{
    if(val->type_ == value::data_type::string
        && !enum_name_.empty())
    {
        return 0;
    }
    return range::validate_type(val, res);
}

int range_enum::validate(value* val, schema::result* res)
{
    int r;
    r = range::validate(val, res);
    if(!r)
    {
        if(val->type_ == value::data_type::string)
        {
            //jgb_assert(!enum_name_.empty());
            int err_count = 0;
            for(int i=0; i<val->len_; i++)
            {
                r = validate(val->str_[i]);
                if(r)
                {
                    int64_t lval;
                    r = val->get(lval);
                    if(!r)
                    {
                        r = validate(lval);
                    }
                }
                put_result(val, i, res, r);
                if(r)
                {
                    ++ err_count;
                }
            }
            return err_count > 0 ? JGB_ERR_SCHEMA_NOT_MATCHED_RANGE : 0;
        }
        else
        {
            int err_count = 0;
            for(int i=0; i<val->len_; i++)
            {
                r = validate(val->int_[i]);
                put_result(val, i, res, r);
                if(r)
                {
                    ++ err_count;
                }
            }
            return err_count > 0 ? JGB_ERR_SCHEMA_NOT_MATCHED_RANGE : 0;
        }
    }
    else
    {
        return r;
    }
}

static int get_part_value(const char* s, const char* e, jgb::value::data_type type, jgb::range::part& part)
{
    // 跳过空格
    while(*s == ' ')
    {
        ++ s;
    }
    if(s < e)
    {
        //jgb_debug("{ s = %.*s }", e - s, s);
        int r;
        if(type == jgb::value::data_type::integer)
        {
            r = jgb::stoll(std::string(s, e - s), part.int64);
        }
        else
        {
            jgb_assert(type == jgb::value::data_type::real);
            r = jgb::stod(std::string(s, e - s), part.real);
        }
        return r;
    }
    return JGB_ERR_NOT_FOUND;
}

static int get_lower_part(const char* p0, const char* p1, jgb::value::data_type type, jgb::range::interval &interval)
{
    int r;
    r = get_part_value(p0 + 1, p1, type, interval.lower);
    //jgb_debug("{ r = %d }", r);
    if(!r)
    {
        if(*p0 == '[')
        {
            interval.lower.inbound = true;
        }
        else
        {
            jgb_assert(*p0 == '(');
            interval.lower.inbound = false;
        }
        interval.lower.has = true;
    }
    return r;
}

static int get_upper_part(const char* p1, const char* p2, jgb::value::data_type type, jgb::range::interval &interval)
{
    int r;
    r = get_part_value(p1 + 1, p2, type, interval.upper);
    if(!r)
    {
        if(*p2 == ']')
        {
            interval.upper.inbound = true;
        }
        else
        {
            jgb_assert(*p2 == ')');
            interval.upper.inbound = false;
        }
        interval.upper.has = true;
    }
    return r;
}

static int scan_one_interval(const char* str, jgb::value::data_type type, jgb::range::interval &interval)
{
    jgb_assert(str);
    interval = {};

    // 扫描区间的开始。
    const char* p0 = str;
    while(*p0 != '\0' && *p0 != '[' && *p0 != '(')
    {
        ++ p0;
    }
    if(*p0 == '\0')
    {
        // 提前结束。
        return JGB_ERR_INVALID;
    }
    // 扫描间隔，或者关闭。
    const char* p1 = p0 + 1;
    while(*p1 != '\0' && *p1 != ']' && *p1 != ')' && *p1 != ',')
    {
        ++ p1;
    }
    // 找到到间隔。
    if(*p1 == ',')
    {
        const char* p2 = p1 + 1;
        // 扫描关闭。
        while(*p2 != '\0' && *p2 != ']' && *p2 != ')')
        {
            ++ p2;
        }
        if(*p2 == '\0')
        {
            // 提前结束。
            return JGB_ERR_INVALID;
        }
        // 找到区间的右部关闭
        if(*p2 == ']' || *p2 == ')')
        {
            int r1;
            r1 = get_lower_part(p0, p1, type, interval);
            if(r1 != JGB_ERR_INVALID)
            {
                int r2;
                r2 = get_upper_part(p1, p2, type, interval);
                if(!r1 || !r2)
                {
                    ++ p2;
                    while(*p2 == ' ')
                    {
                        ++ p2;
                    }
                    if(*p2 == '\0')
                    {
                        return 0;
                    }
                }
            }
            return JGB_ERR_INVALID;
        }
    }
    // 找到区间的左部直接关闭：只有左部。
    else if(*p1 == ']' || *p1 == ')')
    {
        int r = get_lower_part(p0, p1, type, interval);
        if(!r)
        {
            ++ p1;
            while(*p1 == ' ')
            {
                ++ p1;
            }
            if(*p1 == '\0')
            {
                return 0;
            }
        }
    }
    return JGB_ERR_INVALID;
}

static bool validate(int64_t ival, jgb::range::interval& interval)
{
    if(interval.lower.has)
    {
        if(ival < interval.lower.int64)
        {
            return false;
        }
        if(!interval.lower.inbound && ival == interval.lower.int64)
        {
            return false;
        }
    }
    if(interval.upper.has)
    {
        if(ival > interval.upper.int64)
        {
            return false;
        }
        if(!interval.upper.inbound && ival == interval.upper.int64)
        {
            return false;
        }
    }
    return true;
}

range_int::range_int(value *v_size, bool is_required, bool is_array, value* range_val_)
    : range(value::data_type::integer, v_size, is_required, is_array)
{
    jgb_assert(range_val_);
    jgb_assert(range_val_->type_ == value::data_type::string);
    jgb_assert(range_val_->len_ == 1);
    jgb_assert(range_val_->str_[0]);

    intervals_.resize(range_val_->len_);
    int count = 0;
    int r;
    for(int i=0; i<range_val_->len_; i++)
    {
        if(range_val_->str_[i])
        {
            r = scan_one_interval(range_val_->str_[i], jgb::value::data_type::integer, intervals_[i]);
            if(!r)
            {
                ++ count;
            }
            else
            {
                jgb_warning("invalid interval. { text = %s }", range_val_->str_[i]);
            }
        }
    }
    intervals_.resize(count);
}

int range_int::validate(int64_t ival)
{
    //jgb_debug("{ intervals.size() = %d }", intervals_.size());
    for(uint i=0; i<intervals_.size(); i++)
    {
        if(!jgb::validate(ival, intervals_[i]))
        {
            continue;
        }
        return 0;
    }
    return JGB_ERR_SCHEMA_NOT_MATCHED_RANGE;
}

int range_int::validate(value* val, schema::result* res)
{
    int r;
    r = range::validate(val, res);
    if(!r)
    {
        int err_count = 0;
        for(int i=0; i<val->len_; i++)
        {
            r = validate(val->int_[i]);
            put_result(val, i, res, r);
            if(r)
            {
                ++ err_count;
            }
        }
        return err_count > 0 ? JGB_ERR_SCHEMA_NOT_MATCHED_RANGE : 0;
    }
    else
    {
        return r;
    }
}

range_real::range_real(value *v_size, bool is_required, bool is_array, value* range_val_)
    : range(value::data_type::real, v_size, is_required, is_array)
{
    jgb_assert(range_val_);
    jgb_assert(range_val_->type_ == value::data_type::string);
    jgb_assert(range_val_->len_ > 0);
    intervals_.resize(range_val_->len_);

    intervals_.resize(range_val_->len_);
    int count = 0;
    int r;
    for(int i=0; i<range_val_->len_; i++)
    {
        if(range_val_->str_[i])
        {
            r = scan_one_interval(range_val_->str_[i], jgb::value::data_type::real, intervals_[i]);
            if(!r)
            {
                ++ count;
            }
            else
            {
                jgb_warning("invalid interval. { text = %s }", range_val_->str_[i]);
            }
        }
    }
    intervals_.resize(count);
}

int range_real::validate(double rval)
{
    for(uint i=0; i<intervals_.size(); i++)
    {
        if(intervals_[i].lower.has)
        {
            if(rval < intervals_[i].lower.real)
            {
                continue;
            }
            if(!intervals_[i].lower.inbound && jgb::is_equal(rval, intervals_[i].lower.real))
            {
                continue;
            }
        }
        if(intervals_[i].upper.has)
        {
            if(rval > intervals_[i].upper.real)
            {
                continue;
            }
            if(!intervals_[i].upper.inbound && jgb::is_equal(rval, intervals_[i].lower.real))
            {
                continue;
            }
        }
        return 0;
    }
    return JGB_ERR_SCHEMA_NOT_MATCHED_RANGE;
}

int range_real::validate(value* val, schema::result* res)
{
    int r;
    r = range::validate(val, res);
    if(!r)
    {
        int err_count = 0;
        for(int i=0; i<val->len_; i++)
        {
            //jgb_debug("%d: %f", i, val->real_[i]);
            r = validate(val->real(i));
            //jgb_debug("r = %d", r);
            put_result(val, i, res, r);
            if(r)
            {
                ++ err_count;
            }
        }
        return err_count > 0 ? JGB_ERR_SCHEMA_NOT_MATCHED_RANGE : 0;
    }
    else
    {
        return r;
    }
}

struct range_re::Impl
{
    std::vector<pcre2_code*> re_vec_;
};

range_re::range_re(value *v_size, bool is_required, bool is_array, value* range_val_)
    : range(value::data_type::string, v_size, is_required, is_array),
      pimpl_(new Impl())
{
    jgb_function();
    jgb_assert(range_val_);
    jgb_assert(range_val_->type_ == value::data_type::string);
    jgb_assert(range_val_->len_ > 0);
    pimpl_->re_vec_.resize(range_val_->len_);
    int count = 0;
    for(int i = 0; i < range_val_->len_; i++)
    {
        const char* sval = range_val_->str_[i];
        if(sval)
        {
            int errornumber;
            PCRE2_SIZE erroroffset;

            pimpl_->re_vec_[count] = pcre2_compile((PCRE2_SPTR8) sval, PCRE2_ZERO_TERMINATED, 0, &errornumber, &erroroffset, nullptr);
            if (pimpl_->re_vec_[count])
            {
                ++ count;
            }
            else
            {
                PCRE2_UCHAR buffer[256];
                pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
                jgb_warning("PCRE2 compilation failed. { pattern = \"%s\", offset = %d,  message = \"%s\"",
                            sval,
                            (int)erroroffset, buffer);
            }
        }
    }
    pimpl_->re_vec_.resize(count);
    //jgb_debug("{ size = %lu }", pimpl_->re_vec_.size());
}

range_re::~range_re()
{
    for(auto i: pimpl_->re_vec_)
    {
        pcre2_code_free(i);
    }
    pimpl_->re_vec_.clear();
}

int range_re::validate(const char* str)
{
    jgb_function();
    for(auto i: pimpl_->re_vec_)
    {
        pcre2_match_data *match_data;
        match_data = pcre2_match_data_create_from_pattern(i, NULL);
        int r = pcre2_match(i, (PCRE2_SPTR)str, strlen(str), 0, 0, match_data, nullptr);
        //jgb_debug("{ pattern %p, r = %d str = %.*s }", i, r, strlen(str), str);
        pcre2_match_data_free(match_data);
        if(r >= 0)
        {
            return 0;
        }
    }
    return JGB_ERR_SCHEMA_NOT_MATCHED_RANGE;
}

int range_re::validate(value* val, schema::result* res)
{
    int r;
    r = range::validate(val, res);
    if(!r)
    {
        int err_count = 0;
        for(int i=0; i<val->len_; i++)
        {
            r = validate(val->str_[i]);
            put_result(val, i, res, r);
            if(r)
            {
                ++ err_count;
            }
        }
        return err_count > 0 ? JGB_ERR_SCHEMA_NOT_MATCHED_RANGE : 0;
    }
    else
    {
        return r;
    }
}

static void on_found_type(value* val, void* arg)
{
    jgb_assert(arg);
    jgb_assert(val);
    jgb_assert(val->type_ == value::data_type::string);
    if(val->valid_ && val->len_ == 1)
    {
        jgb_assert(val->uplink_); // pair
        jgb_assert(val->uplink_->uplink_); // config
        range* ra = range_factory::create(val->uplink_->uplink_);
        if(ra)
        {
            schema* s = static_cast<schema*>(arg);
            std::string path;
            val->uplink_->uplink_->get_path(path);
            //jgb_debug("new range. { path = %s }", path.c_str());
            jgb_assert(path.length() > 0);
            s->ranges_.insert(std::pair<std::string, range*>(path, ra));
        }
    }
    else
    {
        std::string path;
        val->get_path(path);
        jgb_warning("schema type is invalid { path = %s }", path.c_str());
    }
}

schema::~schema()
{
    for(auto i: ranges_)
    {
        jgb_assert(i.second);
        delete i.second;
    }
}

struct validate_context
{
    range* range_;
    struct schema::result* res_;
};

static void to_validate(value* val, struct validate_context* ctx)
{
    jgb_assert(ctx);
    jgb_assert(ctx->res_);
    jgb_assert(ctx->range_);
    ctx->range_->validate(val, ctx->res_);
}

static void walk_through(const char* path, range* ra, config* conf, struct schema::result* res)
{
    int r;
    const char* s = path;
    const char* e;

    //jgb_debug("{ path = %s }", path);
    r = jpath_parse(&s, &e);
    if(!r)
    {
        jgb_assert(s < e);
        //jgb_debug("{ part = %.*s }", e - s, s);
        jgb::pair* pr = conf->find(s, (int)(e - s));
        // 已经没有子目录。
        if(*e == '\0')
        {
            if(pr)
            {
                validate_context ctx = {ra, res};
                to_validate(pr->value_, &ctx);
            }
            else
            {
                if(ra->is_required_)
                {
                    std::string xpath;
                    conf->get_path(xpath);
                    if(xpath.back() != '/')
                    {
                        xpath += '/';
                    }
                    xpath += std::string(s, e - s);
                    //jgb_debug("param not provided. { path = %s }", xpath.c_str());
                    res->error_.push_back({xpath, JGB_ERR_SCHEMA_NOT_PRESENT});
                }
            }
        }
        // 还有子目录。
        else
        {
            if(pr)
            {
                if(pr->value_->type_ == jgb::value::data_type::object)
                {
                    for(int i=0; i<pr->value_->len_; i++)
                    {
                        if(pr->value_->conf_[i])
                        {
                            walk_through(e, ra, pr->value_->conf_[i], res);
                        }
                    }
                }
            }
        }
    }
    else
    {
        jgb_assert(0);
    }
}

int schema::validate(config* conf, result* res)
{
    if(conf)
    {
        schema::result x_res;
        struct schema::result* p_res = res ? res : &x_res;
        for(auto& i: ranges_)
        {
            walk_through(i.first.c_str(), i.second, conf, p_res);
        }
        return p_res->error_.size() ? JGB_ERR_SCHEMA_NOT_MATCHED : 0;
    }
    return JGB_ERR_INVALID;
}

void schema::dump(const schema::result& res)
{
    jgb_raw("schema validate result:\n");

    jgb_raw("ok:\n");
    if(res.ok_.size())
    {
        for(auto i: res.ok_)
        {
            jgb_raw("  %s\n", i.c_str());
        }
    }
    else
    {
        jgb_raw("  none\n");
    }

    jgb_raw("error:\n");
    if(res.error_.size())
    {
        for(auto i: res.error_)
        {
            jgb_raw("  %d %s\n", i.code, i.path.c_str());
        }
    }
    else
    {
        jgb_raw("  none\n");
    }
}

schema* schema_factory::create(config* c)
{
    if(!c)
    {
        return nullptr;
    }
    schema* s = new schema();
    jgb::find(c, "type", value::data_type::string, on_found_type, s);
    return s;
}

value::data_type get_type(const char* s)
{
    if(!strcmp(s, "int"))
    {
        return value::data_type::integer;
    }
    else if(!strcmp(s, "real"))
    {
        return value::data_type::real;
    }
    else if(!strcmp(s, "string"))
    {
        return value::data_type::string;
    }
    else if(!strcmp(s, "object"))
    {
        return value::data_type::object;
    }
    else
    {
        jgb_error("unknown type. { type = %s }", s);
        return value::data_type::none;
    }
}

range* range_factory::create(config* c)
{
    if(!c)
    {
        return nullptr;
    }

    int r;
    const char* s;
    r = c->get("type", &s);
    if(!r)
    {
        value::data_type type = get_type(s);
        if(type != value::data_type::none)
        {
            value* v_size = nullptr;
            int is_bool = 0;
            int is_array = 0;
            int is_required = 0;

            c->get("required", is_required);
            c->get("size", &v_size);
            c->get("is_bool", is_bool);
            c->get("is_array", is_array);

            if(is_bool)
            {
                range* ra = new range_enum(v_size, is_required, is_array);
                return ra;
            }

            value* val;
            r = c->get("range", &val);
            if(!r)
            {
                if(val->valid_)
                {
                    if(val->type_ == value::data_type::integer
                        && val->len_ > 0)
                    {
                        // enum 枚举型。
                        range* ra = new range_enum(v_size, is_required, is_array, val);
                        return ra;
                    }
                    else if(val->type_ == value::data_type::string
                               && val->len_ > 0)
                    {
                        // 范围型。
                        if(type == value::data_type::integer)
                        {
                            // int 型范围型。
                            range* ra = new range_int(v_size, is_required, is_array, val);
                            return ra;
                        }
                        else if(type == value::data_type::real)
                        {
                            // real 型范围型。
                            range* ra = new range_real(v_size, is_required, is_array, val);
                            return ra;
                        }
                        else if(type == value::data_type::string)
                        {
                            // re - 字符串型正则表达式。
                            range* ra = new range_re(v_size, is_required, is_array, val);
                            return ra;
                        }
                    }
                    else
                    {
                        //std::cout << val << std::endl;
                        jgb_warning("schema range is invalid. { range = %s }", c->to_string().c_str());
                    }
                }
            }
            else
            {
                range* ra = new range(type, v_size, is_required, is_array);
                return ra;
            }
        }
    }
    return nullptr;
}

} // namespace jgb
