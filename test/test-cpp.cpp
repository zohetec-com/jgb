#include <jgb/core.h>
#include <jgb/helper.h>
#include <set>

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
    // FIXME! -- ++ iterator after erased it!
    // 没有任何报错。会引发段错误吗？
    for(auto i = my_set.begin(); i != my_set.end(); ++i)
    {
        if((*i) == ptr1)
        {
            jgb_debug("erase { %p }", *i);
            my_set.erase(i);
        }
    }
    //jgb_assert(0);
}

static int init(void*)
{
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
