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

static void test_get_dir()
{
    const char* path = "/p7///p78[1]/";
    const char* s = path;
    const char* e = nullptr;
    int r;

    while(*s != '\0')
    {
        r = get_dir(&s, &e);
        jgb_assert(!r);
        jgb_debug("%.*s", (int)(e - s), s);
        s = e;
    }

    const char* path2 = nullptr;
    s = path2;
    jgb_assert(get_dir(&s, &e));

    const char* path3 = "";
    s = path3;
    r = get_dir(&s, &e);
    jgb_assert(!r);
    jgb_assert(*s == '\0');
    jgb_assert(*e == '\0');
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

    // 以 json 文档更新 config。
    bool changed = jgb::config_factory::update(conf, "update.json");
    jgb_assert(changed);

    std::cout << "[UPDATED]" << conf << std::endl;

#if 0
    // 异常处理？
    jgb_assert(conf->get_value("/p1").valid());
    jgb_assert(conf->get_value("/p1").type() == jgb::value::data_type::integer);
    jgb_assert(conf->get_value("/p1").size() == 1);
    jgb_assert(conf->get_value("/p1").int_[0] == 123);
    jgb_assert(conf->get_value("/p1").integer() == 123);

    jgb_assert(conf->get_value("/p4").valid());
    jgb_assert(conf->get_value("/p4").type() == jgb::value::data_type::integer);
    jgb_assert(conf->get_value("/p4").size() == 3);
    jgb_assert(conf->get_value("/p4").int_[0] == 1);
    jgb_assert(conf->get_value("/p4").int_[1] == 2);
    jgb_assert(conf->get_value("/p4").int_[2] == 3);

    jgb_assert(conf->get_value("/p7/p71").valid());
    jgb_assert(conf->get_value("/p7/p71").type() == jgb::value::data_type::integer);
    jgb_assert(conf->get_value("/p7/p71").size() == 1);
    jgb_assert(conf->get_value("/p7/p71").int_[0] == 123);
    jgb_assert(conf->get_value("/p7/p71").integer() == 123);

    jgb_assert(conf->get_value("/p7/p4").valid());
    jgb_assert(conf->get_value("/p7/p4").type() == jgb::value::data_type::integer);
    jgb_assert(conf->get_value("/p7/p4").size() == 3);
    jgb_assert(conf->get_value("/p7/p4").int_[0] == 1);
    jgb_assert(conf->get_value("/p7/p4").int_[1] == 2);
    jgb_assert(conf->get_value("/p7/p4").int_[2] == 3);
#endif
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

    test_const();
    test_get_dir();
    test_create_update();
    test_get_dev_info();

    return 0;
}
