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
#include "core.h"
#include <string.h>
#include <jansson.h>
#include <boost/filesystem.hpp>

namespace jgb
{
static int object_type = (1 << JSON_OBJECT);
static int string_type = (1 << JSON_STRING);
static int int_type = (1 << JSON_INTEGER);
static int bool_type = (1 << JSON_TRUE) | (1 << JSON_FALSE);
static int int_real_type = (1 << JSON_INTEGER) | (1 << JSON_REAL);

static config* create_config(json_t* json);
//static bool update_config(config* conf, json_t* json);

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

        //jgb_debug("{ json_type = 0x%02x }", json_type);

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
    //jgb_debug("{ size = %lu }", size);

    if(size > 0)
    {
        value::data_type d_type;
        bool is_bool;

        // TODO：优先使用 shema 定义的数据类型。
        if(json_array_get_type(json, &d_type, &is_bool))
        {
            //jgb_debug("{ d_type = %d, is_bool = %d }", (int) d_type, is_bool);

            jgb::value* val = new jgb::value(d_type, size, true, is_bool);
            val->valid_ = true;

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
                    jgb_assert(!val->str_[i]);
                    json_val = json_array_get(json, i);
                    val->str_[i] = strdup(json_string_value(json_val));
                    jgb_assert(val->str_[i]);
                }
                return val;

            case value::data_type::object:
                for (i = 0; i < size; i++)
                {
                    jgb_assert(val->conf_[i]);
                    delete val->conf_[i];
                    json_val = json_array_get(json, i);
                    val->conf_[i] = create_config(json_val);
                    val->conf_[i]->id_ = i;
                    val->conf_[i]->uplink_ = val;
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
        //jgb_debug("{ key = %s }", key);
        switch (json_typeof(json_val))
        {
            case JSON_OBJECT:
                {
                    jgb::config* cval = create_config(json_val);
                    if(cval)
                    {
                        conf->create(key, cval);
                    }
                }
                break;
            case JSON_ARRAY:
                {
                    jgb::value* val = create_array(json_val);
                    if(val)
                    {
                        conf->create(key, val);
                        if(val->type_ == jgb::value::data_type::none)
                        {
                            jgb_debug("{ jpath =  %s }",
                                      key);
                        }
                    }
                }
                break;
            case JSON_STRING:
                conf->set(key, json_string_value(json_val));
                break;
            case JSON_INTEGER:
                conf->set(key, (int64_t) json_integer_value(json_val));
                break;
            case JSON_REAL:
                conf->set(key, json_real_value(json_val));
                break;
            case JSON_TRUE:
            case JSON_FALSE:
                conf->set(key, json_typeof(json_val) == JSON_TRUE, true, true);
                break;
            case JSON_NULL:
                conf->create(key);
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
#if 0
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
        jgb_fail("decode data. { data = %.*s, code = %d, error = \"%s\", line = %d, column = %d, position = %d, source = \"%s\" }",
                 len, buf, json_error_code(&error), error.text,
                 error.line, error.column, error.position, error.source);
        return nullptr;
    }
}

static int check_file_path(const char* file_path, std::string& path)
{
    if(!file_path || !file_path[0])
    {
        return JGB_ERR_INVALID;
    }

    std::string path_x = std::string(file_path);
    if(boost::filesystem::exists(path_x))
    {
        path = path_x;
        return 0;
    }

    if(file_path[0] != '/')
    {
        path_x = std::string(jgb::core::get_instance()->conf_dir()) + '/' + path_x;
        if(boost::filesystem::exists(path_x))
        {
            path = path_x;
            return 0;
        }
    }

    jgb_debug("文件不存在。{ file_path = %s }", file_path);

    return JGB_ERR_NOT_FOUND;
}

config* config_factory::create(const char* file_path)
{
    int r;
    std::string path;

    r = check_file_path(file_path, path);
    if(r)
    {
        return nullptr;
    }

    json_t* json;
    json_error_t error;

    json = json_load_file(path.c_str(), 0, &error);
    if(json)
    {
        config* conf = jgb::create(json);
        json_decref(json);
        return conf;
    }
    else
    {
        jgb_fail("decode file. { file = %s, code = %d, error = \"%s\", line = %d, column = %d, position = %d, source = \"%s\" }",
                 path.c_str(), json_error_code(&error), error.text,
                 error.line, error.column, error.position, error.source);
        return nullptr;
    }
}
} // namespace jgb
