#include "config_factory.h"
#include <string.h>
#include <jansson.h>

namespace jgb
{
static int object_type = (1 << JSON_OBJECT);
static int string_type = (1 << JSON_STRING);
static int int_type = (1 << JSON_INTEGER);
static int bool_type = (1 << JSON_TRUE) | (1 << JSON_FALSE);
static int int_real_type = (1 << JSON_INTEGER) | (1 << JSON_REAL);

static config* create_config(json_t* json);

static bool json_array_get_type(json_t* json, value::data_type* type, bool* is_bool = NULL)
{
    jgb_assert(json);
    jgb_assert(type);

    if(json_typeof(json) == JSON_ARRAY)
    {
        size_t i;
        size_t size = json_array_size(json);
        int json_type = 0;
        bool is_type = true;
        bool is_bool_ = false;

        for (i = 0; i < size; i++)
        {
            int t_ = json_typeof(json_array_get(json, i));
            if(t_ != JSON_NULL)
            {
                json_type |= (1 << t_);
            }
        }

        // 要求数组元素同构:
        //     -- 不支持混用 object,array,string,integer/real，true/false。
        // 暂不支持多维数组；
        if(json_type == (json_type & object_type))
        {
            *type = value::data_type::object;
        }
        else if(json_type == (json_type & string_type))
        {
            *type = value::data_type::string;
        }
        else if(json_type == (json_type & int_type))
        {
            *type = value::data_type::integer;
        }
        else if((json_type & bool_type) && !(json_type & ~bool_type))
        {
            *type = value::data_type::integer;
            is_bool_ = true;
            jgb_debug("bool array");
        }
        else if((json_type & int_real_type) && !(json_type & ~int_real_type))
        {
            *type = value::data_type::real;
        }
        else
        {
            jgb_debug("不兼容的数组结构：");
            return false;
        }

        if(is_bool)
        {
            *is_bool = is_bool_;
        }
        return true;
    }
    else
    {
        jgb_assert(0);
    }
    return false;
}

static jgb::value* create_array(json_t* json)
{
    size_t size = json_array_size(json);

    jgb_assert(json_typeof(json) == JSON_ARRAY);

    if(size > 0)
    {
        value::data_type d_type;
        bool is_bool;

        // TODO：优先使用 shema 定义的数据类型。
        if(json_array_get_type(json, &d_type, &is_bool))
        {
            jgb::value* val = new jgb::value(d_type, size, true, is_bool);

            size_t i;
            json_t *json_val;

            switch (d_type)
            {
            case value::data_type::integer:
                for (i = 0; i < size; i++)
                {
                    json_val = json_array_get(json, i);
                    if(!is_bool)
                    {
                        val->int_[i] = json_integer_value(json_val);
                    }
                    else
                    {
                        val->int_[i] = json_typeof(json_val) == JSON_TRUE;
                    }
                }
                return val;

            case value::data_type::real:
                for (i = 0; i < size; i++)
                {
                    json_val = json_array_get(json, i);
                    if(json_typeof(json_val) == JSON_REAL)
                    {
                        val->real_[i] = json_real_value(json_val);
                    }
                    else
                    {
                        // 可能是 null。
                        //jgb_assert(json_typeof(json_val) == JSON_INTEGER);
                        val->real_[i] = json_integer_value(json_val);
                    }
                }
                return val;

            case value::data_type::string:
                for (i = 0; i < size; i++)
                {
                    json_val = json_array_get(json, i);
                    val->str_[i] = strdup(json_string_value(json_val));
                    jgb_assert(val->str_[i]);
                }
                return val;

            case value::data_type::object:
                for (i = 0; i < size; i++)
                {
                    json_val = json_array_get(json, i);
                    val->conf_[i] = create_config(json_val);
                    jgb_assert(val->conf_[i]);
                }
                return val;
            }
        }
    }
    else
    {
        // TODO：如果 schema 有定义，空数组也可以。
        jgb_warning("空数组");
        return nullptr;
    }
}

static config* create_config(json_t* json)
{
    jgb_assert(json);
    jgb_assert(json_typeof(json) == JSON_OBJECT);

    config* conf = new config;

    const char *key;
    json_t *json_val;

    json_object_foreach(json, key, json_val)
    {
        switch (json_typeof(json_val))
        {
            case JSON_OBJECT:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::object);
                val->conf_[0] = create_config(json_val);
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_ARRAY:
            {
                jgb::value* val = create_array(json_val);
                if(val)
                {
                    conf->conf_.push_back(new pair(key, val));
                }
            }
                break;
            case JSON_STRING:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::string);
                val->str_[0] = strdup(json_string_value(json_val));
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_INTEGER:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::integer);
                val->int_[0] = json_integer_value(json_val);
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_REAL:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::real);
                val->real_[0] = json_real_value(json_val);
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_TRUE:
            case JSON_FALSE:
            {
                // TODO：结合 schema 信息使用，适配参数（需要）使用别名的场景。
                jgb::value* val = new jgb::value(jgb::value::data_type::integer, 1, false, true);
                val->int_[0] = json_typeof(json_val) == JSON_TRUE;
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_NULL:
                // TODO: 结合 schema 信息使用，适配参数没有初始值的场景。
                jgb_warning("忽略 JSON type NULL.\n");
                break;
            default:
                jgb_warning("unrecognized JSON type %d\n", json_typeof(json_val));
        }
    }

    return conf;
}

static config* create(json_t* json)
{
    config* conf = nullptr;
    jgb_assert(json);
#ifdef DEBUG
    char* json_text = json_dumps(json, 0);
    jgb_debug("json: %s", json_text);
    free(json_text);
#endif
    if(json_typeof(json) == JSON_OBJECT)
    {
        conf = create_config(json);
    }
    else
    {
        jgb_error("不支持的 JSON 文档格式");
    }
    return conf;
}

config* config_factory::create(const char* buf, int len)
{
    if(!buf)
    {
        return nullptr;
    }

    json_t* json;
    json_error_t error;

    json = json_loadb(buf, len, 0, &error);
    if(json)
    {
        config* conf = jgb::create(json);
        json_decref(json);
        return conf;
    }
    else
    {
        jgb_fail("decode: %.*s", len, buf);
        return nullptr;
    }
}

config* config_factory::create(const char* file_path)
{
    if(!file_path)
    {
        return nullptr;
    }

    json_t* json;
    json_error_t error;

    json = json_load_file(file_path, 0, &error);
    if(json)
    {
        config* conf = jgb::create(json);
        json_decref(json);
        return conf;
    }
    else
    {
        jgb_fail("decode: %s", file_path);
        return nullptr;
    }
}

//
static bool compatible(value* val, json_t* json)
{

}

static bool update_array(value* val, json_t* json)
{
    return false;
}
// 发送更新消息：被更新的参数的路径，及新值？
// 返回值：false-没有触发更新，true-触发更新。
static bool update_config(config* conf, json_t* json)
{
    jgb_assert(conf);
    jgb_assert(json);
    jgb_assert(json_typeof(json) == JSON_OBJECT);

    const char *key;
    json_t *json_val;
    int count = 0;

    json_object_foreach(json, key, json_val)
    {
        pair* pr = conf->find(key);
        if(pr)
        {
            switch (json_typeof(json_val))
            {
                case JSON_OBJECT:
                    {
                        if(pr->value_->type_ == value::data_type::object)
                        {
                            jgb_assert(pr->value_->conf_[0]);
                            count += update_config(pr->value_->conf_[0], json_val);
                        }
                    }
                    break;
                case JSON_ARRAY:
                    {
                        count += update_array(pr->value_, json_val);
                    }
                    break;
                case JSON_STRING:
                    if(pr->value_->type_ == value::data_type::string)
                    {
                        const char* json_str = json_string_value(json_val);
                        jgb_assert(pr->value_->str_[0]);
                        if(strcmp(pr->value_->str_[0], json_str) != 0)
                        {
                            // TODO：如果长度变小且变化不大，避免重新内存分配。考虑使用 realloc() 吗？
                            free((void*)pr->value_->str_[0]);
                            pr->value_->str_[0] = strdup(json_str);
                            ++ count;
                        }
                    }
                    break;
                case JSON_INTEGER:
                    {
                        int64_t json_int = json_integer_value(json_val);

                        if(pr->value_->type_ == value::data_type::integer)
                        {
                            if(pr->value_->int_[0] != json_int)
                            {
                                pr->value_->int_[0] = json_integer_value(json_val);
                                ++ count;
                            }
                        }
                        // 以整型更新浮点数，应该没什么问题。
                        else if(pr->value_->type_ == value::data_type::real)
                        {
                            if(pr->value_->real_[0] != json_int)
                            {
                                pr->value_->real_[0] = json_integer_value(json_val);
                                ++ count;
                            }
                        }
                    }
                    break;
                case JSON_REAL:
                    {
                        double json_real = json_real_value(json_val);

                        if(pr->value_->type_ == value::data_type::real)
                        {
                            if(pr->value_->real_[0] != json_real)
                            {
                                pr->value_->real_[0] = json_real_value(json_val);
                                ++ count;
                            }
                        }
                        else if(pr->value_->type_ == value::data_type::integer)
                        {
                            jgb_warning("以浮点型更新整型数据，精度损失风险。key %s", key);
                            if(pr->value_->int_[0] != json_real)
                            {
                                pr->value_->int_[0] = json_real_value(json_val);
                                ++ count;
                            }
                        }
                    }
                    break;
                case JSON_TRUE:
                case JSON_FALSE:
                    {
                        int64_t json_int = json_typeof(json_val) == JSON_TRUE;

                        if(pr->value_->type_ == value::data_type::integer
                                && pr->value_->is_bool_)
                        {
                            if(pr->value_->int_[0] != json_int)
                            {
                                pr->value_->int_[0] = json_int;
                                ++ count;
                            }
                        }
                        else
                        {
                            jgb_warning("type unmatch: key %s", key);
                        }
                    }
                    break;
                case JSON_NULL:
                    jgb_warning("忽略 JSON type NULL.\n");
                    break;
                default:
                    jgb_warning("unrecognized JSON type %d\n", json_typeof(json_val));
            }
        }
        else
        {
            jgb_debug("not found: key = %s", key);
        }
    }

    return count > 0;
}

bool config_factory::update(config* conf, const char* buf, int len)
{
    if(!conf || !buf)
    {
        return false;
    }

    json_t* json;
    json_error_t error;

    json = json_loadb(buf, len, 0, &error);
    if(json)
    {
        bool ret;

        ret = update_config(conf, json);
        json_decref(json);
        return ret;
    }
    else
    {
        jgb_fail("decode: %.*s", len, buf);
        return false;
    }
}

bool config_factory::update(config* conf, const char* file_path)
{
    if(!conf || !file_path)
    {
        return false;
    }

    json_t* json;
    json_error_t error;

    json = json_load_file(file_path, 0, &error);
    if(json)
    {
        bool ret;

        ret = update_config(conf, json);
        json_decref(json);
        return ret;
    }
    else
    {
        jgb_fail("decode: %s", file_path);
        return false;
    }
}
} // namespace jgb
