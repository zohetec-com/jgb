#include <jgb/core.h>
#include <jgb/helper.h>
#include <set>
#include <jgb/log.h>
#include "write_32u_context.h"
#include "check_u32_context.h"

class XA
{
public:
    int x1_;

    XA()
    {
        x1_ = 12345678;
    }
};

struct SS
{
    jgb::write_32u_context wr_ctx;
    jgb::check_u32_context chk_ctx[2];
    XA x_xa;
};

static void test_constructor()
{
    struct SS* x_ss = new SS;
    jgb_assert(x_ss->x_xa.x1_ == 12345678);
    jgb_assert(x_ss->wr_ctx.serial_ == 0);
    jgb_assert(x_ss->wr_ctx.pos_ == 0);
    delete x_ss;
}

static void test_set_remove()
{
    std::set<void*> my_set;
    void* ptr0 = (void*)0xcdef;
    void* ptr1 = (void*)0x1234;
    void* ptr2 = (void*)0x5678;
    void* ptr3 = (void*)0x9abc;
    my_set.insert(ptr0);
    my_set.insert(ptr1);
    my_set.insert(ptr2);
    my_set.insert(ptr3);
    for(auto i = my_set.begin(); i != my_set.end(); ++i)
    {
        if((*i) == ptr1)
        {
            jgb_debug("erase { %p }", *i);
            my_set.erase(i);
            // 如果不 return：
            // 1 ++ iterator after erased it!
            // 2 导致未定义的行为！
            // 3 可引发段错误。不一定会发生，但确实会发生。
            return;
        }
    }
}

static int init(void*)
{
    test_constructor();
    test_set_remove();
    return 0;
}

jgb_api_t test_cpp
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test cpp",
    .init = init,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = nullptr
};
