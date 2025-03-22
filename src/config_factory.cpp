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
static bool update_config(config* conf, json_t* json);

// 1 要求数组元素的数据类型相同:
//   - 不支持混用 object,array,string,integer/real,true/false,null。
// 2 暂不支持多维数组；
static bool json_array_get_type(json_t* json, value::data_type* type, bool* is_bool = NULL)
{
    jgb_assert(json);
    jgb_assert(type);

    if(json_typeof(json) == JSON_ARRAY)
    {
        size_t i;
        size_t size = json_array_size(json);
        int json_type = 0;
        bool is_bool_ = false;

        for (i = 0; i < size; i++)
        {
            int t_ = json_typeof(json_array_get(json, i));
            json_type |= (1 << t_);
        }

        jgb_debug("{ json_type = 0x%02x }", json_type);

        if(json_type == object_type)
        {
            *type = value::data_type::object;
        }
        else if(json_type == string_type)
        {
            *type = value::data_type::string;
        }
        else if(json_type == int_type)
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
            jgb_debug("不兼容的数组类型：{ json_type = 0x%x }", json_type);
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
        return false;
    }
}

static jgb::value* create_array(json_t* json)
{
    size_t size = json_array_size(json);

    jgb_assert(json_typeof(json) == JSON_ARRAY);
    jgb_debug("{ size = %lu }", size);

    if(size > 0)
    {
        value::data_type d_type;
        bool is_bool;

        // TODO：优先使用 shema 定义的数据类型。
        if(json_array_get_type(json, &d_type, &is_bool))
        {
            jgb_debug("{ d_type = %d, is_bool = %d }", (int) d_type, is_bool);

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
            default:
                jgb_assert(0);
            }
        }
        else
        {
            jgb_warning("不兼容的数组类型");
        }
        return nullptr;
    }
    else
    {
        jgb::value* val = new jgb::value(value::data_type::none, 0, true);
        jgb_debug("创建空数组");
        return val;
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
        jgb_bug("{ key = %s }", key);
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
                {
                    jgb::value* val = new jgb::value(jgb::value::data_type::none);
                    conf->conf_.push_back(new pair(key, val));
                }
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

// 判断 value 的数据类型与 json 数组是否兼容。
static bool is_compatibe_type(value* val, json_t* json)
{
    value::data_type d_type;
    bool is_bool;

    // TODO：优先使用 shema 定义的数据类型。
    if(json_array_get_type(json, &d_type, &is_bool))
    {
        jgb_debug("{ d_type = %d, val->type_ %d }", (int) d_type, (int) val->type_);
        switch (val->type_)
        {
        case value::data_type::none:
            return true;
        case value::data_type::integer:
            return d_type == value::data_type::integer && is_bool == val->is_bool_;
        case value::data_type::real:
            return d_type == value::data_type::integer || d_type == value::data_type::real;
        case value::data_type::string:
            return d_type == value::data_type::string;
        case value::data_type::object:
            return d_type == value::data_type::object;
        }
    }
    return false;
}

static bool update_array(value* val, json_t* json)
{
    int size = json_array_size(json);

    jgb_assert(json_typeof(json) == JSON_ARRAY);
    if(!val->is_array_)
    {
        return false;
    }

    if(!is_compatibe_type(val, json))
    {
        jgb_debug("数组数据类型不兼容。");
        return false;
    }

    int count = 0;
    if(val->len_ && size == val->len_)
    {
        int i;
        json_t *json_val;

        switch (val->type_)
        {
        case value::data_type::integer:
            for (i = 0; i < size; i++)
            {
                int64_t new_int;
                json_val = json_array_get(json, i);
                if(!val->is_bool_)
                {
                    new_int = json_integer_value(json_val);
                }
                else
                {
                    new_int = json_typeof(json_val) == JSON_TRUE;
                }
                if(val->int_[i] != new_int)
                {
                    jgb_debug("%ld => %ld", val->int_[i], new_int);
                    val->int_[i] = new_int;
                    ++ count;
                }
            }
            break;
        case value::data_type::real:
            for (i = 0; i < size; i++)
            {
                double new_real;
                json_val = json_array_get(json, i);
                if(json_typeof(json_val) == JSON_REAL)
                {
                    new_real = json_real_value(json_val);
                }
                else
                {
                    // 还可能是 null。
                    //jgb_assert(json_typeof(json_val) == JSON_INTEGER);
                    new_real = json_integer_value(json_val);
                }
                jgb_bug("%f => %f", val->real_[i], new_real);
                if(val->real_[i] != new_real)
                {
                    jgb_bug("%f => %f", val->real_[i], new_real);
                    val->real_[i] = new_real;
                    ++ count;
                }
            }
            break;
        case value::data_type::string:
            for (i = 0; i < size; i++)
            {
                json_val = json_array_get(json, i);
                const char* new_str = json_string_value(json_val);
                jgb_assert(new_str);
                jgb_assert(val->str_[i]);
                if(strcmp(val->str_[i], new_str))
                {
                    jgb_debug("%s => %s", val->str_[i], new_str);
                    free((void*) val->str_[i]);
                    val->str_[i] = strdup(json_string_value(json_val));
                    jgb_assert(val->str_[i]);
                    ++ count;
                }
            }
            break;
        case value::data_type::object:
            for (i = 0; i < size; i++)
            {
                json_val = json_array_get(json, i);
                count += update_config(val->conf_[i], json_val);
            }
            break;
        default:
            jgb_assert(0);
        }
    }
#if 0
    else if(!val->len_ && size)
    {

    }
#endif
    else
    {
        jgb_debug("数组长度不匹配。{ val->len_ = %d, json size = %d}", val->len_, size);
        // TODO: 空数组。
        // 代表什么意思？清除还是占位符？
    }
    return count > 0;
}
// 发送更新消息：被更新的参数的路径，及新值？
// 返回值：false-没有触发更新，true-触发更新。
static bool update_config(config* conf, json_t* json)
{
#ifdef DEBUG
    char* json_text = json_dumps(json, 0);
    jgb_debug("json: %s", json_text);
    free(json_text);
#endif

    jgb_assert(conf);
    jgb_assert(json);
    jgb_assert(json_typeof(json) == JSON_OBJECT);

    const char *key;
    json_t *json_val;
    int count = 0;

    json_object_foreach(json, key, json_val)
    {
        jgb_debug("{ key = %s, count = %d }", key, count);
        pair* pr = conf->find(key);
        if(pr)
        {
            if(pr->value_->type_ != value::data_type::none)
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
                            if(update_array(pr->value_, json_val))
                            {
                                ++ count;
                            }
                            else
                            {
                                jgb_debug("数组未更新。{ key = %s }", key);
                            }
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
                            jgb_debug("{ json_real = %f, type = %d, size = %d, pr->real = %f }",
                                      json_real,
                                      (int) pr->value_->type_, pr->value_->len_,
                                       pr->value_->real_[0]);

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
                                jgb_error("禁止使用浮点型数据更新整型数据。存在精度损失风险。{ key = %s }", key);
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
                                jgb_warning("type unmatch. { key = %s }", key);
                            }
                        }
                        break;
                    case JSON_NULL:
                        // TODO: 应该如何处理 NULL 值？
                        // 1) 当作占位符，忽略。
                        // 2) 当做清除请求，清除对应参数的取值，使参数处于 unset 未初始化的状态。-- 谨慎起见不建议采用。
                        jgb_warning("忽略 JSON type NULL.\n");
                        break;
                    default:
                        jgb_warning("unrecognized JSON type %d\n", json_typeof(json_val));
                }
            }
            else
            {
                // TODO:
                // 2025.3.20: 还没有想好如何实现。
                jgb_warning("todo. { key = %s }", key);
            }
        }
        else
        {
            jgb_debug("not found. { key = %s }", key);
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
