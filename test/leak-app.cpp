// 产生内存泄漏。
#include <jgb/core.h>
#include <jgb/helper.h>

static int tsk_leak(void*)
{
    char* p;
    p = (char*) malloc(4);
    p[0] = 0;
    p = new char[4];
    p[0] = 1;
    jgb::sleep(1000);
    return 0;
}

static loop_ptr_t loops[] = { tsk_leak, nullptr };

static jgb_loop_t loop
{
    .setup = nullptr,
    .loops = loops,
    .exit = nullptr
};

jgb_api_t leak_app
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "template app",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
