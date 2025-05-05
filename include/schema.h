#ifndef SCHEMA_H_20250503
#define SCHEMA_H_20250503

#include <string>
#include <list>
#include <map>
#include <vector>
#include <jgb/config.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace jgb
{

class range
{
public:
    range(value::data_type type, int len, bool is_array, bool is_bool);
    virtual ~range();

    value::data_type type_;
    int len_;
    bool is_array_;
    bool is_bool_;

    virtual int validate(value* val);
};

class range_enum : public range
{
public:
    range_enum(int len, bool is_array, bool is_bool, value* range_val_);

    int validate(value* val) override;
    int validate(int ival);

    std::vector<int> enum_int_;
};

class range_re : public range
{
public:
    range_re(int len, bool is_array, value* range_val_);
    ~range_re();

    int validate(const char* str);
    int validate(value* val) override;

private:
    std::vector<pcre2_code*> res_;
};

class range_int : public range
{
public:
    range_int(int len, bool is_array, bool is_bool, value* range_val_);

    int validate(value* val) override;
    int validate(int ival);

    struct interval
    {
        bool has_upper;
        bool has_lower;
        int upper;
        int lower;
    };
    std::vector<struct interval> intervals_;
};

class range_real : public range
{
public:
    range_real(int len, bool is_array, value* range_val_);

    int validate(value* val) override;
    int validate(double rval);

    struct interval
    {
        bool has_upper;
        bool has_lower;
        double upper;
        double lower;
    };
    std::vector<struct interval> intervals_;
};

class schema
{
public:
    struct error
    {
        std::string path;
        int code;
    };

    struct result
    {
        std::list<std::string> ok_;
        std::list<struct error> error_;
        std::list<std::string> no_schema_;
    };

    ~schema();

    int validate(config* conf, struct result* res = nullptr);

    std::map<std::string,range*> ranges_;
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
