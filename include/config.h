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
#include <jgb/error.h>
#include <jgb/log.h>

namespace jgb
{
class value;
class pair;
class config;

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
    value(const value& other);
    friend void swap(value& a, value& b);
    value& operator=(value);
    ~value();

    double to_real(int idx = 0);

    int get(const char* path, value** val, int* idx=nullptr);

    // 返回 value 的 jpath。
    // 如果 idx 取值非0且有效，则在路径末尾添加 "[$idx]"。
    // show_idx_0 为 true 时，如果 value 为数组，即使 idx=0，仍在路径末尾添加 "[0]"。适用于报告参数检查结果场景。
    void get_path(std::string& path, int idx = 0, bool show_idx_0 = false);

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

    static const int object_len_max = 1024;
};

class pair
{
public:
    pair(const char* name, value* value, config* uplink);
    pair(const pair& other);
    friend void swap(pair& a, pair& b);
    pair& operator=(pair);
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
    config(const config& other);
    friend void swap(config& a, config& b);
    config& operator=(config);

    ~config();

    // 返回 config 的 jpath。
    void get_path(std::string& path);

    void clear();

    // n 用于限定 name 的长度，以配合 jpath 使用。
    pair* find(const char* name, int n = 0) const;

    int64_t int64(const char* path);
    std::string str(const char* path);
    double real(const char* path);

    int get(const char* path, value** val, int* idx = nullptr);
    int get(const char* path, bool& bval);
    int get(const char* path, int& ival);
    int get(const char* path, int64_t& lval);
    int get(const char* path, double& rval);
    int get(const char* path, const char** sval);
    int get(const char* path, std::string& sval);
    int get(const char* path, config** cval);

    int set(const char* path, bool bval);
    int set(const char* path, int ival);
    int set(const char* path, int64_t lval);
    int set(const char* path, double rval);
    int set(const char* path, const char* sval);
    int set(const char* path, const std::string& sval);

    int create(const char* name, config* cval);
    int create(const char* name);
    int create(const char* name, value* val);
    int create(const char* name, bool bval);
    int create(const char* name, int ival, bool is_bool = false);
    int create(const char* name, int64_t lval, bool is_bool = false);
    int create(const char* name, double rval);
    int create(const char* name, const char* sval);
    int create(const char* name, const std::string& sval);

    int remove(const char* name);

    friend std::ostream& operator<<(std::ostream& os, const config* conf);

    std::string to_string();

public:
    // 不使用 map，因为输出文本需要匹配原始输入文本顺序。
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

int update(config* dest, config* src, std::list<std::string>* diff = nullptr, bool dry_run = false);
int update(value* dest, value* src, std::list<std::string>* diff = nullptr, bool dry_run = false);

void find(value* v,  const std::string& name, value::data_type type, void(*on_found)(value*,void*), void* arg = nullptr);
void find(config* c, const std::string& name, value::data_type type, void(*on_found)(value*,void*), void* arg = nullptr);

} // namespace jgb

#endif // CONFIG_H
