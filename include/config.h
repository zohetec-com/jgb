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
        none,
        integer,
        real,
        string,
        object
    };

    value(data_type type = data_type::none,
          int len = 1,
          bool is_array = false,
          bool is_bool = false,
          pair* uplink = nullptr);
    ~value();

    int get(const char* path, value** val);
    // 返回 value 的 jpath。
    void get_path(std::string& path);

    friend std::ostream& operator<<(std::ostream& os, const value* val);

    union
    {
        int64_t* int_;
        double* real_;
        const char** str_;
        config** conf_;
    };

    data_type type_;
    // 是否支持改变长度？
    //  -- 预料大部分的场景不需要。
    //  -- 为减小资源占用，避免直接使用 std::vector。


    int len_;

    // 指示 int_/real_ 中存储的内容是否有效：
    //   - true：有效，内容可以使用。
    //   - false：无效，内容不可使用。
    // 应用场景1：
    //   - 在规格文件中定义参数 p 的类型为 int，在配置文件中设置 p 初值为 null。
    //   - 期望：所创建的 p 参数的 valid_ 应为 false。
    bool valid_;

    // TODO:优先采用 schema 定义。
    // 处理长度为 1 的情况。
    bool is_array_;
    bool is_bool_;

    // for jpath.
    pair* uplink_;
};

class pair
{
public:
    pair(const char* name, value* value, config* uplink);
    ~pair();

    // 返回 pair 的 jpath。
    void get_path(std::string& path);

    friend std::ostream& operator<<(std::ostream& os, const pair* pr);

public:
    const char* name_;
    value* value_;

    // for jpath.
    config* uplink_;
};

class config
{
public:
    config(value* uplink = nullptr, int id = 0);
    ~config();

    // 返回 config 的 jpath。
    void get_path(std::string& path);

    void clear();

    pair* find(const char* name, int n = 0);

    int get(const char* path, value** val);
    int get(const char* path, int& ival);
    int get(const char* path, double& rval);
    int get(const char* path, const char** sval);
    int get(const char* path, std::string& sval);
    int get(const char* path, config** cval);

    int set(const char* name, int64_t ival, bool create = true, bool is_bool = false);
    int set(const char* name, double rval, bool create = true);
    int set(const char* name, const char* sval, bool create = true);
    int set(const char* name, const std::string& sval, bool create = true);
    int create(const char* name, config* cval);
    int create(const char* name);
    int create(const char* name, value* val);

    friend std::ostream& operator<<(std::ostream& os, const config* conf);

    std::string to_string();

public:
    std::list<pair*> pair_;

    // for jpath.
    value* uplink_;
    int id_;

private:
    // 根据 path 查找 val，及返回 path 所指定的索引号。
    // 如果 path = "/a[2]", 则返回由 "/a" 确定的 val，及返回 idx = 2。
    // 调用者得到 val 后，如果 val 是一个数组，可自行从 val 获取元素。
    // 是实现其他 get() 接口的基础。
    // 注意：
    // val,idx 都仅是输出参数。不要误会以为 idx 是输入参数。
    // 由 path 所确定的 val 只有一个，所以没得选择。
    // val 可以是一个数组，但是数组的成员不是 value 类型，所以无法选择一个成员作为 val 返回。
    int get(const char* path, value** val, int& idx);
};

}

#endif // CONFIG_H
