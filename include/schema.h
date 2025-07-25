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
#ifndef SCHEMA_H_20250503
#define SCHEMA_H_20250503

#include <string>
#include <list>
#include <map>
#include <vector>
#include <jgb/config.h>

namespace jgb
{
class range;

class schema
{
public:
    struct error
    {
    public:
        std::string path;
        int code;
    };

    struct result
    {
    public:
        std::list<std::string> ok_;
        std::list<struct error> error_;
    };

    static void dump(const result& res);

    ~schema();

    int validate(config* conf, result* res = nullptr);

    std::map<std::string,range*> ranges_;
};

class range
{
public:
    struct part
    {
        bool has;
        bool inbound;
        union
        {
            int64_t int64;
            double real;
        };
    };

    struct interval
    {
        struct part lower;
        struct part upper;
    };

    range(value::data_type type, value *v_size, bool is_required, bool is_array);
    virtual ~range();

    value::data_type type_;
    int len_;
    struct interval* inter_len_;
    bool required_;
    bool array_;

    virtual int validate_type(value* val, schema::result* res);
    virtual int validate(value* val, schema::result* res = nullptr);
};

class range_enum : public range
{
public:
    range_enum(value *v_size, bool is_required, bool is_array, value* range_val_);
    // for bool
    range_enum(value *v_size, bool is_required, bool is_array);

    int validate_type(value* val, schema::result* res) override;
    int validate(value* val, schema::result* res = nullptr) override;
    int validate(int ival);
    int validate(const char* str);

    std::vector<int> enum_int_;
    std::vector<std::string> enum_name_;
};

class range_re : public range
{
public:
    range_re(value *v_size, bool is_required, bool is_array, value* range_val_);
    ~range_re();

    int validate(const char* str);
    int validate(value* val, schema::result* res = nullptr) override;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class range_int : public range
{
public:
    range_int(value *v_size, bool is_required, bool is_array, value* range_val_);

    int validate(value* val, schema::result* res = nullptr) override;
    int validate(int64_t ival);

    std::vector<struct interval> intervals_;
};

class range_real : public range
{
public:
    range_real(value *v_size, bool is_required, bool is_array, value* range_val_);

    int validate(value* val, schema::result* res = nullptr) override;
    int validate(double rval);

    std::vector<struct interval> intervals_;
};

class schema_factory
{
public:
    static schema* create(config* conf);
};

class range_factory
{
public:
    static range* create(config* conf);
};

} // namespace jgb

#endif // SCHEMA_H_20250503
