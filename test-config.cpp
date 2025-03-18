#include "helper.h"
#include "config_factory.h"

int test_get_dir()
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

    return 0;
}

int main()
{
    test_get_dir();

    jgb::config* conf = jgb::config_factory::create("test.json");

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
    return 0;
}
