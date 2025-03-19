#include "config_factory.h"
#include <string.h>
#include <jansson.h>

namespace jgb
{
static config* create_config(json_t* json);

static jgb::value* create_array(json_t* json)
{
    size_t i;
    size_t size = json_array_size(json);

    jgb_assert(json_typeof(json) == JSON_ARRAY);

    if(size > 0)
    {
        // TODO：优先使用 shema 定义的数据类型。如果 schema 未定义，则从第一个数组成员取得。

        // 约定：数组中的数据应当为单一数据类型。
        json_t *json_sub;
        json_type type0 = json_typeof(json_array_get(json, 0));
        for (i = 1; i < size; i++)
        {
            json_sub = json_array_get(json, i);
            if(json_typeof(json_sub) != type0
                    && ((type0 == JSON_TRUE || type0 == JSON_FALSE)
                        && (json_typeof(json_sub) != JSON_TRUE && json_typeof(json_sub) != JSON_FALSE)))
            {
                jgb_warning("数组中的数据不是单一类型");
                return nullptr;
            }
        }

        jgb_assert(i == size);

        switch (type0) {
        case JSON_OBJECT:
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::object, size, true);
            for (i = 0; i < size; i++)
            {
                json_sub = json_array_get(json, i);
                val->conf_[i] = create_config(json_sub);
                jgb_assert(val->conf_[i]);
            }
            return val;
        }
        case JSON_ARRAY:
        {
            // 暂不支持。
            jgb_warning("不支持数组中的数组");
            return nullptr;
        }
        case JSON_STRING:
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::string, size, true);
            for (i = 0; i < size; i++)
            {
                json_sub = json_array_get(json, i);
                val->str_[i] = strdup(json_string_value(json_sub));
                jgb_assert(val->str_[i]);
            }
            return val;
        }
        case JSON_INTEGER:
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::integer, size, true);
            for (i = 0; i < size; i++)
            {
                json_sub = json_array_get(json, i);
                val->int_[i] = json_integer_value(json_sub);
            }
            return val;
        }
        case JSON_REAL:
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::real, size, true);
            for (i = 0; i < size; i++)
            {
                json_sub = json_array_get(json, i);
                val->real_[i] = json_real_value(json_sub);
            }
            return val;
        }
        case JSON_TRUE:
        case JSON_FALSE:
        {
            jgb::value* val = new jgb::value(jgb::value::data_type::integer, size, true, true);
            for (i = 0; i < size; i++)
            {
                json_sub = json_array_get(json, i);
                val->int_[i] = json_typeof(json_sub) == JSON_TRUE;
            }
            return val;
        }
        case JSON_NULL:
            jgb_warning("忽略 JSON type NULL.\n");
            return nullptr;
        default:
            jgb_warning("unrecognized JSON type %d\n", type0);
            return nullptr;
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
    json_t *json_sub;

    json_object_foreach(json, key, json_sub)
    {
        switch (json_typeof(json_sub))
        {
            case JSON_OBJECT:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::object);
                val->conf_[0] = create_config(json_sub);
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_ARRAY:
            {
                jgb::value* val = create_array(json_sub);
                if(val)
                {
                    conf->conf_.push_back(new pair(key, val));
                }
            }
                break;
            case JSON_STRING:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::string);
                val->str_[0] = strdup(json_string_value(json_sub));
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_INTEGER:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::integer);
                val->int_[0] = json_integer_value(json_sub);
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_REAL:
            {
                jgb::value* val = new jgb::value(jgb::value::data_type::real);
                val->real_[0] = json_real_value(json_sub);
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_TRUE:
            case JSON_FALSE:
            {
                // TODO：结合 schema 信息使用，适配参数（需要）使用别名的场景。
                jgb::value* val = new jgb::value(jgb::value::data_type::integer);
                val->int_[0] = json_typeof(json_sub) == JSON_TRUE;
                conf->conf_.push_back(new pair(key, val));
            }
                break;
            case JSON_NULL:
                // TODO: 结合 schema 信息使用，适配参数没有初始值的场景。
                jgb_warning("忽略 JSON type NULL.\n");
                break;
            default:
                jgb_warning("unrecognized JSON type %d\n", json_typeof(json_sub));
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
}
