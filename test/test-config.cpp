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
#include <jgb/helper.h>
#include <jgb/config_factory.h>
#include <jgb/app.h>
#include <jgb/schema.h>
#include <jgb/core.h>
#include <boost/thread.hpp>

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

static void test_object_value()
{
    jgb::value* val = new jgb::value(jgb::value::data_type::object, 2);
    val->conf_[0]->set("a", 1L);
    std::ostringstream os;
    os << val;
    jgb_debug("%s", os.str().c_str());
    delete val;
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

static void test_compare()
{
    // 验证：比较整型数和浮点数。
    int64_t int_v = 2;
    double  real_v = 2.0;

    jgb_assert(int_v == real_v);

    real_v = 2.1;
    jgb_assert(int_v != real_v);
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

static void test_stox_fail()
{
    std::string str = "abc";
    int r;
    double rv;
    int iv;
    int64_t lv;
    r = jgb::stod(str, rv);
    jgb_assert(r);
    r = jgb::stoi(str, iv);
    jgb_assert(r);
    r = jgb::stoll(str, lv);
    jgb_assert(r);
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

static void check_test_json(jgb::config* conf)
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

    r = conf->get("/p27/x", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1000);

    r = conf->get("/p27/y", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "abc"));

    r = conf->get("/p27[1]/x", ival);
    jgb_assert(!r);
    jgb_assert(ival == 2000);

    r = conf->get("/p27[1]/y", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "def"));

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

    r = conf->get("/p7/p78/version", ival);
    jgb_assert(!r);
    jgb_assert(ival == 2204);

    r = conf->get("/p7/p78[0]/version", ival);
    jgb_assert(!r);
    jgb_assert(ival == 2204);

    r = conf->get("/p7/p78/os", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "ubuntu"));

    r = conf->get("/p7/p78[0]/os", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "ubuntu"));

    r = conf->get("/p7/p78[1]/version", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1210);

    r = conf->get("/p7/p78[1]/os", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "debian"));

    jgb::config* cval;
    r = conf->get("/p7", &cval);
    jgb_assert(!r);

    r = cval->get("p71", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1234);

    r = conf->get("a", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "123"));

    r = conf->get("a1", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "456"));
}

static void test_get()
{
    jgb::config* conf = jgb::config_factory::create("test.json");
    check_test_json(conf);
    delete conf;
}

static void test_get_path()
{
    jgb::config* conf = jgb::config_factory::create("test.json");
    int r;
    jgb::value* val;

    {
        std::string path;
        conf->get_path(path);
        jgb_assert(path == "/");
    }

    r = conf->get("/p1", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p1");
    }

    r = conf->get("/p2", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p2");
    }

    r = conf->get("/p3", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p3");
    }

    r = conf->get("/p7/p71", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p71");
    }

    r = conf->get("/p7/p72", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p72");
    }

    r = conf->get("/p7/p73", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p73");
    }

    r = conf->get("/p7/p74", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p74");
    }

    r = conf->get("/p7/p75", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p75");
    }

    r = conf->get("/p7/p76", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p76");
    }

    r = conf->get("/p7/p77", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p77");
    }

    r = conf->get("/p7/p78", &val);
    jgb_assert(!r);
    jgb_assert(val->type_ == jgb::value::data_type::object);
    jgb_assert(val->len_ == 2);
    jgb_assert(val->valid_);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p78");
    }

    r = conf->get("/p7/p78/version", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p78/version");
    }

    r = conf->get("/p7/p78[0]/version", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p78/version");
    }

    r = conf->get("/p7/p78/os", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p78/os");
    }

    r = conf->get("/p7/p78[0]/os", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p78/os");
    }

    r = conf->get("/p7/p78[1]/version", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p78[1]/version");
    }

    r = conf->get("/p7/p78[1]/os", &val);
    jgb_assert(!r);

    {
        std::string path;
        val->get_path(path);
        //jgb_debug("{ path = %s }", path.c_str());
        jgb_assert(path == "/p7/p78[1]/os");
    }

    delete conf;
}

static void test_find()
{
    // 从 json 文档创建 config 对象。
    jgb::config* conf = jgb::config_factory::create("test.json");
    std::cout << "[ORIGIN]" << conf << std::endl;

    jgb::pair* pr = conf->find("p8");
    jgb_assert(pr);
    jgb_assert(pr->value_->type_ == jgb::value::data_type::real);

    delete conf;
}

static void test_update()
{
    jgb::config* conf = jgb::config_factory::create("test.json");
    jgb::config* conf2 = jgb::config_factory::create("update.json");
    int r;
    std::list<std::string> changed;

    update(conf, conf2, &changed);

    int ival;
    const char* sval;
    double rval;

    r = conf->get("/p1", ival);
    jgb_assert(!r);
    jgb_assert(ival == 456);

    r = conf->get("/p2", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 3.14159265359));

    r = conf->get("/p3", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "def"));

    r = conf->get("/p4[0]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 7);

    r = conf->get("/p4[1]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 8);

    r = conf->get("/p4[2]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 9);

    r = conf->get("/p5[0]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 10.11));

    r = conf->get("/p5[1]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 10.22));

    r = conf->get("/p5[2]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 10.33));

    r = conf->get("/p6[0]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "sh"));

    r = conf->get("/p6[1]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "longlonglonglonglong"));

    r = conf->get("/p7/p71", ival);
    jgb_assert(!r);
    jgb_assert(ival == 4321);

    r = conf->get("/p7/p72", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 4.13));

    r = conf->get("/p7/p73", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "xyz"));

    r = conf->get("/p7/p74[0]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    r = conf->get("/p7/p74[1]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 2);

    r = conf->get("/p7/p74[2]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 3);

    r = conf->get("/p7/p75[0]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 0.1));

    r = conf->get("/p7/p75[1]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 0.2));

    r = conf->get("/p7/p75[2]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 0.3));

    r = conf->get("/p7/p76[0]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "hello"));

    r = conf->get("/p7/p76[1]", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "world"));

    r = conf->get("/p7/p77", ival);
    jgb_assert(!r);
    jgb_assert(ival == 0);

    r = conf->get("/p7/p77[1]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 0);

    r = conf->get("/p7/p78/version", ival);
    jgb_assert(!r);
    jgb_assert(ival == 2404);

    r = conf->get("/p7/p78/os", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "ubuntu"));

    r = conf->get("/p7/p78[1]/version", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1210);

    r = conf->get("/p7/p78[1]/os", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "kylin"));

    r = conf->get("/p8[0]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 99));

    r = conf->get("/p8[1]", rval);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rval, 10));

    // 不变
    r = conf->get("/p9", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    // 不变
    r = conf->get("/p10", ival);
    jgb_assert(!r);
    jgb_assert(ival == 0);

    r = conf->get("/p11", ival);
    jgb_assert(!r);
    jgb_assert(ival == 0);

    r = conf->get("/p12", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    r = conf->get("/p13", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    r = conf->get("/p13[1]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 0);

    r = conf->get("/p14", ival);
    jgb_assert(!r);
    jgb_assert(ival == 1);

    r = conf->get("/p14[1]", ival);
    jgb_assert(!r);
    jgb_assert(ival == 0);

    r = conf->get("/p27/x", ival);
    jgb_assert(!r);
    jgb_assert(ival == 10000);

    r = conf->get("/p27/y", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "abc"));

    r = conf->get("/p27[1]/x", ival);
    jgb_assert(!r);
    jgb_assert(ival == 2000);

    r = conf->get("/p27[1]/y", &sval);
    jgb_assert(!r);
    jgb_assert(!strcmp(sval, "nono"));

    jgb_debug("{ changed = %lu }", changed.size());
    jgb_assert(changed.size() == 33);
#if 0
    int n = 0;
    fprintf(stderr, "{\n");
    for(auto i: changed)
    {
        if(n)
        {
            fprintf(stderr, ",\n");
        }
        fprintf(stderr, "    \"%s\"", i.c_str());
        ++ n;
    }
    fprintf(stderr, "\n}");
#endif
    const char* expected_changed [] =
        {
            "/p1",
            "/p2",
            "/p3",
            "/p4",
            "/p4[1]",
            "/p4[2]",
            "/p5",
            "/p5[1]",
            "/p5[2]",
            "/p6",
            "/p6[1]",
            "/p7/p71",
            "/p7/p72",
            "/p7/p73",
            "/p7/p74",
            "/p7/p74[1]",
            "/p7/p74[2]",
            "/p7/p75",
            "/p7/p75[1]",
            "/p7/p75[2]",
            "/p7/p76",
            "/p7/p76[1]",
            "/p7/p77",
            "/p7/p78/version",
            "/p7/p78[1]/os",
            "/p8",
            "/p8[1]",
            "/p11",
            "/p12",
            "/p14",
            "/p14[1]",
            "/p27/x",
            "/p27[1]/y"
    };

    int n = 0;
    for(auto i: changed)
    {
        jgb_assert(i == std::string(expected_changed[n]));
        ++ n;
    }

    delete conf;
    delete conf2;
}

static void test_get_dev_info()
{
    jgb::config* conf = jgb::config_factory::create("get_dev_info.json");
    std::cout << "[get_dev_info]" << conf << std::endl;
    delete conf;
}

static void test_create()
{
    jgb::config conf;
    int r;
    r = conf.create("type", "request");
    jgb_assert(!r);
    r = conf.create("id", "abc123");
    jgb_assert(!r);
    r = conf.create("mts", time(NULL));
    jgb_assert(!r);
    r = conf.create("command", "get /wsapi/v1/cvcam/sync/get_dev_info");
    jgb_assert(!r);
    r = conf.create("pai", 3.14);
    jgb_assert(!r);
    r = conf.create("str", std::string("hello"));
    jgb_assert(!r);
    std::cout << &conf << std::endl;
}

static void test_invalid()
{
    const char* buf = "invalid";
    jgb::config* conf = jgb::config_factory::create(buf, strlen(buf));
    jgb_assert(!conf);
}

static void test_schema()
{
    jgb::config* schema_conf = jgb::config_factory::create("test-data.schema");
    jgb::config* conf = jgb::config_factory::create("test-data-1.json");
    jgb::schema* schema = jgb::schema_factory::create(schema_conf);
    jgb::schema::result res;
    int r;
    jgb_assert(schema);
    r = schema->validate(conf, &res);
    jgb::schema::dump(res);
    jgb_assert(!r);
    // 没有错的。
    jgb_assert(!res.error_.size());
    delete schema_conf;
    delete conf;
    delete schema;
}

static void test_schema_2()
{
    jgb::config* schema_conf = jgb::config_factory::create("test-data.schema");
    jgb::config* conf = jgb::config_factory::create("test-data-2.json");
    jgb::schema* schema = jgb::schema_factory::create(schema_conf);
    jgb::schema::result res;
    int r;
    jgb_assert(schema);
    r = schema->validate(conf, &res);
    jgb::schema::dump(res);
    // 没有对的。
    jgb_assert(r);
    jgb_assert(!res.ok_.size());
    delete schema_conf;
    delete conf;
    delete schema;
}

static void test_conf()
{
    jgb::config* conf = new jgb::config;
    int r;
    int iv;
    double rv;
    conf->create("a", 20250524);
    iv = conf->int64("a");
    jgb_assert(iv == 20250524);
    iv = 0;
    r = conf->get("a", iv);
    jgb_assert(!r);
    jgb_assert(iv == 20250524);
    r = conf->remove("a");
    jgb_assert(!r);
    std::cout << conf << std::endl;
    r = conf->get("a", iv);
    jgb_debug("r = %d", r);
    jgb_assert(r);
    r = conf->remove("a");
    jgb_assert(r);
    r = conf->create("b", 0.1234);
    jgb_assert(!r);
    rv = conf->real("b");
    jgb_assert(jgb::is_equal(rv, 0.1234));
    rv = 0;
    r = conf->get("b", rv);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rv, 0.1234));
    r = conf->set("b", 0.4321);
    jgb_assert(!r);
    r = conf->get("b", rv);
    jgb_assert(!r);
    jgb_assert(jgb::is_equal(rv, 0.4321));
    r = conf->set("b", "hello");
    jgb_assert(r);
    r = conf->set("b", 1234);
    jgb_assert(!r);
    r = conf->get("b", iv);
    jgb_assert(!r);
    jgb_assert(iv == 1234);
    bool bv;
    r = conf->create("c", 1, true);
    jgb_assert(!r);
    r = conf->get("c", bv);
    jgb_assert(!r);
    jgb_assert(bv == 1);
    r = conf->set("c", 0);
    jgb_assert(!r);
    r = conf->get("c", bv);
    jgb_assert(!r);
    jgb_assert(!bv);
    r = conf->create("d", "hello world");
    jgb_assert(!r);
    std::string sv;
    sv = conf->str("d");
    jgb_assert(!strcmp(sv.c_str(), "hello world"));
    jgb_debug("{ conf = %s }", conf->to_string().c_str());
    delete conf;
}

static void test_set()
{
    jgb::config* c = new jgb::config;
    jgb::value* val = new jgb::value(jgb::value::data_type::object, 2);
    int r;
    r = val->conf_[0]->create("x", 123);
    jgb_assert(!r);
    r = val->conf_[1]->create("x", 345);
    jgb_assert(!r);
    c->create("a", val);
    jgb_assert(val->len_ == 2);
    jgb_assert(c->int64("a[0]/x") == 123);
    jgb_assert(c->int64("a[1]/x") == 345);
    c->set("a[0]/x", 567);
    c->set("a[1]/x", 890);
    jgb_assert(c->int64("a[0]/x") == 567);
    jgb_assert(c->int64("a[1]/x") == 890);
    delete c;
}

static void test_copy()
{
    jgb::config* c1 = jgb::config_factory::create("test.json");
    jgb::config c2 = *c1;
    check_test_json(&c2);
    jgb::config* c3 = new jgb::config(*c1);
    check_test_json(c3);
    check_test_json(c1);
    jgb::config* c4 = jgb::config_factory::create("update.json");
    *c4 = *c1;
    check_test_json(c4);
    delete c1;
    delete c3;
    delete c4;
    jgb::value* v1 = new jgb::value(jgb::value::data_type::integer, 2);
    jgb::value* v2 = new jgb::value(jgb::value::data_type::integer, 2);
    *v2 = *v1;
    jgb::pair p1("a", v1, nullptr);
    jgb::pair p2("b", v2, nullptr);
    p2 = p2;
}

static int init(void*)
{
    // 检查 assert(0) 是否工作。
    //jgb_assert(0);

    test_copy();
    test_create();
    test_conf();
    test_schema_2();
    test_schema();
    test_compare();
    test_invalid();
    test_set();
    test_null_conf();
    test_datatype();
    test_object_value();
    test_value();
    test_null_value();
    test_get_base_index();
    test_stox_fail();
    test_stoi();
    test_const();
    test_jpath_parse();

    test_find();
    test_get();
    test_get_path();
    test_update();

    test_get_dev_info();

    return 0;
}

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    int r = w->task_->instance_->conf_->create("test", "hello");
    jgb_assert(!r);
    return 0;
}

static int tsk_set(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    int n = std::rand() % 100;
    if(n == 0)
    {
        n = 100;
    }
    char str[101];
    int i;
    for(i=0; i<n; i++)
    {
        str[i] = 'x';
    }
    str[i] = '\0';
    w->task_->instance_->lock();
    int r = w->task_->instance_->conf_->set("test", str);
    jgb_assert(!r);
    w->task_->instance_->unlock();
    return 0;
}

static int tsk_get(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    std::string str;
    w->task_->instance_->lock_shared();
    int r = w->task_->instance_->conf_->get("test", str);
    jgb_assert(!r);
    w->task_->instance_->unlock_shared();
    return 0;
}

static int tsk_get_v2(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    std::string str;
    boost::shared_lock<boost::shared_mutex> lock { *static_cast<boost::shared_mutex*>(w->task_->instance_->get_mutex()) };
    int r = w->task_->instance_->conf_->get("test", str);
    jgb_assert(!r);
    return 0;
}

static loop_ptr_t loops[] = { tsk_set, tsk_get, tsk_get, tsk_get_v2, nullptr };

static jgb_loop_t loop
    {
        .setup = tsk_init,
        .loops = loops,
        .exit = nullptr
    };

jgb_api_t test_config
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test config",
    .init = init,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
