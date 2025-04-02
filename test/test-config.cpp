#include "helper.h"
#include "config_factory.h"

static void test_null_conf()
{
    jgb::config conf;
    std::cout << &conf << std::endl;
}

static void test_datatype()
{
    jgb_debug("{ sizeof(int64_t*) = %ld}", sizeof(int64_t*));
    jgb_debug("{ sizeof(jgb::value::data_type) = %ld}", sizeof(jgb::value::data_type));
    jgb_debug("{ sizeof(int) = %ld}", sizeof(int));
    jgb_debug("{ sizeof(bool) = %ld}", sizeof(bool));
    jgb_debug("{ sizeof(int64_t) = %ld}", sizeof(int64_t));

    int64_t x = 0x0001020304050607;
    jgb_debug("{ x = %ld}", x);
}

static void test_value()
{
    jgb::value val;
    jgb_debug("{ sizeof(value) = %lu }", sizeof(val));

    jgb::value* pval = (jgb::value*) 0;
    jgb_debug("{ offset int_ %u }", (uint) (intptr_t) &pval->int_);
    jgb_debug("{ offset type_ %u }", (uint) (intptr_t) &pval->type_);
    jgb_debug("{ offset len_ %u }", (uint) (intptr_t) &pval->len_);
    jgb_debug("{ offset valid_ %u }", (uint) (intptr_t) &pval->valid_);
    jgb_debug("{ offset is_array_ %u }", (uint) (intptr_t) &pval->is_array_);
    jgb_debug("{ offset is_bool_ %u }", (uint) (intptr_t) &pval->is_bool_);
}

static void test_null_value()
{
    static jgb::value* null_val = (jgb::value*) "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    jgb_debug("{ null_val->valid_ = %d }", null_val->valid_);
    //null_val->valid_ = 1;
}

// https://stackoverflow.com/questions/62814873/is-it-possible-to-make-variable-truly-read-only-in-c
static void test_const()
{
    static const jgb::value val_null;
    jgb::value* val = (jgb::value*) &val_null;
    jgb_assert(!val->valid_);
    val->valid_ = true;
    jgb_assert(val->valid_);
}

static void test_jpath_parse()
{
    const char* s;
    const char* e;
    int r;

    {
        const char* path = "/p7///p78[1]/";

        s = path;
        r = jgb::jpath_parse(&s, &e);
        jgb_assert(!r);
        jgb_debug("%.*s", (int)(e - s), s);
        jgb_assert(!strncmp("p7", s, (int)(e - s)));

        s = e;
        r = jgb::jpath_parse(&s, &e);
        jgb_assert(!r);
        jgb_debug("%.*s", (int)(e - s), s);
        jgb_assert(!strncmp("p78", s, (int)(e - s)));

        s = e;
        r = jgb::jpath_parse(&s, &e);
        jgb_assert(!r);
        jgb_debug("%.*s", (int)(e - s), s);
        jgb_assert(!strncmp("[1]", s, (int)(e - s)));

        s = e;
        r = jgb::jpath_parse(&s, &e);
        jgb_assert(!r);
        jgb_debug("%.*s", (int)(e - s), s);
        jgb_assert(s == e);
    }

    const char* path2 = nullptr;
    s = path2;
    jgb_assert(jgb::jpath_parse(&s, &e));

    const char* path3 = "";
    s = path3;
    r = jgb::jpath_parse(&s, &e);
    jgb_assert(!r);
    jgb_assert(*s == '\0');
    jgb_assert(*e == '\0');
}

static void test_stoi()
{
    int v;
    int r;

    r = jgb::stoi("0x1234", v);
    jgb_assert(!r);
    jgb_assert(v == 0x1234);

    r = jgb::stoi("1234", v);
    jgb_assert(!r);
    jgb_assert(v == 1234);

    r = jgb::stoi("1", v);
    jgb_assert(!r);
    jgb_assert(v == 1);
}

static void test_get_base_index()
{
    {
        const char* str = "/a/b/c[3]";
        std::string base;
        int idx = -1;
        int r;
        r = jgb::get_base_index(str, base, idx);
        jgb_assert(!r);
        jgb_assert(idx == 3);
        jgb_assert(!strcmp(base.c_str(), "/a/b/c"));
    }

    {
        const char* str = "[1]";
        std::string base;
        int idx = -1;
        int r;
        r = jgb::get_base_index(str, base, idx);
        jgb_assert(!r);
        jgb_assert(idx == 1);
        jgb_assert(!strcmp(base.c_str(), ""));
    }
}

static void test_get(jgb::config* conf)
{
    int r;
    jgb::value* val;

    r = conf->get("/p1", &val);
    jgb_assert(!r);
    jgb_assert(val->type_ == jgb::value::data_type::integer);
    jgb_assert(val->len_ == 1);
    jgb_assert(val->int_[0] == 123);
    jgb_assert(val->valid_);

    int ival;
    const char* sval;
    double rval;

    r = conf->get("/p1", ival);
    jgb_assert(!r);
    jgb_assert(ival == 123);

    r = conf->get("/p2", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 3.14));

    r = conf->get("/p3", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "abc"));

    r = conf->get("/p4", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    r = conf->get("/p4[0]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    r = conf->get("/p4[1]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 2);

    r = conf->get("/p4[2]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 3);

    r = conf->get("/p5", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 0.1));

    r = conf->get("/p5[0]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 0.1));

    r = conf->get("/p5[1]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 0.2));

    r = conf->get("/p5[2]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 0.3));

    r = conf->get("/p6", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "hello"));

    r = conf->get("/p6[0]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "hello"));

    r = conf->get("/p6[1]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "world"));

    r = conf->get("/p7/p71", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1234);

    r = conf->get("/p7/p72", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 31.4));

    r = conf->get("/p7/p73", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "abcdefg"));

    r = conf->get("/p7/p74", ival);
    jgb_assert(!r);
    jgb_assert(ival == 10);

    r = conf->get("/p7/p74[0]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 10);

    r = conf->get("/p7/p74[1]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 20);

    r = conf->get("/p7/p74[2]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 30);

    r = conf->get("/p7/p75", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 1.0));

    r = conf->get("/p7/p75[0]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 1.0));

    r = conf->get("/p7/p75[1]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 2.0));

    r = conf->get("/p7/p75[2]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 3.0));

    r = conf->get("/p7/p76", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "asia"));

    r = conf->get("/p7/p76[0]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "asia"));

    r = conf->get("/p7/p76[1]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "pacific"));

    r = conf->get("/p7/p77", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    r = conf->get("/p7/p77[0]", ival);
    jgb_assert(!r);
    jgb_assert(ival);

    r = conf->get("/p7/p77[1]", ival);
    jgb_assert(!r);
    jgb_assert(!ival);

    r = conf->get("/p7/p78", &val);
    jgb_assert(!r);
    jgb_assert(val->type_ == jgb::value::data_type::object);
    jgb_assert(val->len_ == 2);
    jgb_assert(val->valid_);

    jgb::config* cval;
    r = conf->get("/p7", &cval);
    jgb_assert(!r);

    r = cval->get("p71", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1234);
}

static void test_create_update()
{
    // 从 json 文档创建 config 对象。
    jgb::config* conf = jgb::config_factory::create("test.json");
    std::cout << "[ORIGIN]" << conf << std::endl;

    jgb::pair* pr = conf->find("p8");
    jgb_assert(pr);
    jgb_assert(pr->value_->type_ == jgb::value::data_type::real);

    // 验证：比较整型数和浮点数。
    {
        int64_t int_v = 2;
        double  real_v = 2.0;

        jgb_assert(int_v == real_v);

        real_v = 2.1;
        jgb_assert(int_v != real_v);
    }

    test_get(conf);

    // 以 json 文档更新 config。
    bool changed = jgb::config_factory::update(conf, "update.json");
    jgb_assert(changed);

    std::cout << "[UPDATED]" << conf << std::endl;

    delete conf;
}

static void test_get_dev_info()
{
    jgb::config* conf = jgb::config_factory::create("get_dev_info.json");
    std::cout << "[get_dev_info]" << conf << std::endl;
    delete conf;
}

int main()
{
    // 检查 assert(0) 是否工作。
    //jgb_assert(0);

    test_null_conf();
    test_datatype();
    test_value();
    test_null_value();

    test_get_base_index();
    test_stoi();
    test_const();
    test_jpath_parse();
    test_create_update();
    test_get_dev_info();

    return 0;
}
