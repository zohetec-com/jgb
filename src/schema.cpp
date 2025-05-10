#include "schema.h"
#include "helper.h"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace jgb
{

range::range(value::data_type type, int len, bool is_array, bool is_bool)
    : type_(type),
    len_(len),
    is_array_(is_array),
    is_bool_(is_bool)
{
}

range::~range()
{
}

static void put_result(value* val, int idx, schema::result *res, int code)
{
    jgb_assert(val);
    jgb_debug("res = %p", res);
    if(res)
    {
        std::string path;
        val->get_path(path, idx, false, true);
        jgb_debug("{ code = %d, path = %s }", code, path.c_str());
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

int range::validate(value* val, schema::result* res)
{
    if(!val)
    {
        return JGB_ERR_INVALID;
    }

    if(val->type_ != type_)
    {
        if(val->type_ == value::data_type::integer && type_ == value::data_type::real)
        {
            // compatible
        }
        else
        {
            put_result(val, 0, res, JGB_ERR_SCHEMA_NOT_MATCHED_TYPE);
            return JGB_ERR_SCHEMA_NOT_MATCHED_TYPE;
        }
    }

    if(val->len_ != len_)
    {
        put_result(val, 0, res, JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH);
        return JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH;
    }

    return 0;
}

range_enum::range_enum(int len, bool is_array, bool is_bool, value* range_val_)
    : range(value::data_type::integer,len,is_array,is_bool)
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
            jgb_debug("valid int. { ival = %ld }", ival);
            return 0;
        }
    }
    return JGB_ERR_SCHEMA_NOT_MATCHED_RANGE;
}

int range_enum::validate(value* val, schema::result* res)
{
    int r;
    if(val->type_ == value::data_type::string
        && !enum_name_.empty())
    {
        if(val->len_ != len_)
        {
            put_result(val, 0, res, JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH);
            return JGB_ERR_SCHEMA_NOT_MATCHED_LENGTH;
        }
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
}

range_int::range_int(int len, bool is_array, bool is_bool, value* range_val_)
    : range(value::data_type::integer, len, is_array, is_bool)
{
    jgb_assert(range_val_);
    jgb_assert(range_val_->type_ == value::data_type::object);
    jgb_assert(range_val_->len_ > 0);
    intervals_.resize(range_val_->len_);
    int count = 0;
    for(int i = 0; i < range_val_->len_; i++)
    {
        config* c = range_val_->conf_[i];
        jgb_assert(c);
        int upper,lower;
        bool has_upper,has_lower;
        int r;
        r = c->get("upper", upper);
        has_upper = !r;
        r = c->get("lower", lower);
        has_lower = !r;
        intervals_[count].has_upper = has_upper;
        intervals_[count].has_lower = has_lower;
        intervals_[count].upper = upper;
        intervals_[count].lower = lower;
        if(has_lower || has_upper)
        {
            ++ count;
        }
        else
        {
            jgb_warning("invalid interval");
        }
    }
    intervals_.resize(count);
}

int range_int::validate(int ival)
{
    jgb_debug("{intervals.size() = %d }", intervals_.size());
    for(uint i=0; i<intervals_.size(); i++)
    {
        if(intervals_[i].has_lower && intervals_[i].has_upper)
        {
            jgb_debug("{ ival = %ld, lower = %ld, upper = %ld }", ival, intervals_[i].lower, intervals_[i].upper);
            if(ival >= intervals_[i].lower && ival <= intervals_[i].upper)
            {
                return 0;
            }
        }
        else if(intervals_[i].has_lower)
        {
            jgb_debug("{ ival = %ld, lower = %ld }", ival, intervals_[i].lower);
            if(ival >= intervals_[i].lower)
            {
                return 0;
            }
        }
        else if(intervals_[i].has_upper)
        {
            jgb_debug("{ ival = %ld, upper = %ld }", ival, intervals_[i].upper);
            if(ival <= intervals_[i].upper)
            {
                return 0;
            }
        }
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

range_real::range_real(int len, bool is_array, value* range_val_)
    : range(value::data_type::real, len, is_array, false)
{
    jgb_assert(range_val_);
    jgb_assert(range_val_->type_ == value::data_type::object);
    jgb_assert(range_val_->len_ > 0);
    intervals_.resize(range_val_->len_);
    int count = 0;
    for(int i = 0; i < range_val_->len_; i++)
    {
        config* c = range_val_->conf_[i];
        jgb_assert(c);
        double upper,lower;
        bool has_upper,has_lower;
        int r;
        r = c->get("upper", upper);
        has_upper = r == 0;
        r = c->get("lower", lower);
        has_lower = r == 0;
        intervals_[count].has_upper = has_upper;
        intervals_[count].has_lower = has_lower;
        intervals_[count].upper = upper;
        intervals_[count].lower = lower;
        if(has_lower || has_upper)
        {
            ++ count;
        }
        else
        {
            jgb_warning("invalid interval");
        }
    }
    intervals_.resize(count);
}

int range_real::validate(double rval)
{
    for(uint i=0; i<intervals_.size(); i++)
    {
        if(intervals_[i].has_lower && intervals_[i].has_upper)
        {
            jgb_debug("{ rval = %f, lower = %f, upper = %f }", rval, intervals_[i].lower, intervals_[i].upper);
            if((rval > intervals_[i].lower || is_equal(rval, intervals_[i].lower))
                && (rval < intervals_[i].upper || is_equal(rval, intervals_[i].upper)))
            {
                return 0;
            }
        }
        else if(intervals_[i].has_lower)
        {
            jgb_debug("{ rval = %f, lower = %f }", rval, intervals_[i].lower);
            if(rval > intervals_[i].lower || is_equal(rval, intervals_[i].lower))
            {
                return 0;
            }
        }
        else if(intervals_[i].has_upper)
        {
            jgb_debug("{ rval = %f, upper = %f }", rval, intervals_[i].upper);
            if(rval < intervals_[i].upper || is_equal(rval, intervals_[i].upper))
            {
                return 0;
            }
        }
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
            r = validate(val->to_real(i));
            jgb_debug("r = %d", r);
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

range_re::range_re(int len, bool is_array, value* range_val_)
    : range(value::data_type::string, len, is_array, false),
      pimpl_(new Impl())
{
    jgb_function();
    jgb_assert(range_val_);
    jgb_assert(range_val_->type_ == value::data_type::object);
    jgb_assert(range_val_->len_ > 0);
    pimpl_->re_vec_.resize(range_val_->len_);
    int count = 0;
    for(int i = 0; i < range_val_->len_; i++)
    {
        const char* sval;
        int r;
        config* c = range_val_->conf_[i];
        jgb_assert(c);

        r = c->get("re", &sval);
        if(!r)
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
    jgb_debug("{ size = %lu }", pimpl_->re_vec_.size());
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
        jgb_debug("{ pattern %p, r = %d str = %.*s }", i, r, strlen(str), str);
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
            val->uplink_->uplink_->get_path(path, true);
            jgb_debug("new range. { path = %s }", path.c_str());
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
    struct schema::result* res_;
    schema* s_;
};

static void to_validate(value* val, void* arg)
{
    std::string schema_path;
    struct validate_context* ctx = (struct validate_context*) arg;
    jgb_assert(ctx);
    jgb_assert(ctx->res_);
    jgb_assert(ctx->s_);
    val->get_path(schema_path, 0, true);
    auto i = ctx->s_->ranges_.find(schema_path);
    if(i != ctx->s_->ranges_.end())
    {
        jgb_assert(i->second);
        i->second->validate(val, ctx->res_);
    }
    else
    {
        jgb_debug("schema not found. { schema_path = %s }", schema_path);
    }
}

int schema::validate(config* conf, result* res)
{
    if(conf)
    {
        struct validate_context ctx;
        schema::result x_res;
        ctx.s_ = this;
        ctx.res_ = res ? res : &x_res;
        find_attr(conf, to_validate, &ctx);
        return ctx.res_->error_.size() > 0 ? JGB_ERR_SCHEMA_NOT_MATCHED : 0;
    }
    else
    {
        return JGB_ERR_INVALID;
    }
}

void schema::dump(const schema::result& res)
{
    jgb_raw("schema validate result:\n");

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
            int size = 1;
            int is_bool = 0;
            int is_array;
            c->get("size", size);
            c->get("is_bool", is_bool);
            if(size < 2)
            {
                is_array = 0;
                c->get("is_array", is_array);
            }
            else
            {
                is_array = 1;
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
                        range* ra = new range_enum(size, is_array, is_bool, val);
                        return ra;
                    }
                    else if(val->type_ == value::data_type::object
                        && val->len_ > 0)
                    {
                        // 范围型。
                        if(type == value::data_type::integer)
                        {
                            // int 型范围型。
                            range* ra = new range_int(size, is_array, is_bool, val);
                            return ra;
                        }
                        else if(type == value::data_type::real)
                        {
                            // real 型范围型。
                            range* ra = new range_real(size, is_array, val);
                            return ra;
                        }
                        else if(type == value::data_type::string)
                        {
                            // re - 字符串型正则表达式。
                            range* ra = new range_re(size, is_array, val);
                            return ra;
                        }
                    }
                    else
                    {
                        jgb_warning("schema range is invalid { path = %s }", "range");
                    }
                }
            }
            else
            {
                range* ra = new range(type, size, is_array, is_bool);
                return ra;
            }
        }
    }
    return nullptr;
}

} // namespace jgb
