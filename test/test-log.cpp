#include <jgb/core.h>

static int forward_to_vsnprintf(char* buf, int len, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buf, len, format, args);
    va_end(args);
    return n;
}

// 确认 vsnprintf 返回长度不包含 '\0'。
static void test_vsnprintf()
{
    char buf[128];
    int n = forward_to_vsnprintf(buf, 128, "123456");
    jgb_assert(n == 6);
}

static void test_long_log()
{
    char buf[2048];
    memset(buf, 'A', 2048);
    buf[2047] = '\0';
    jgb_debug("%s", buf);
}

static int init(void*)
{
    test_vsnprintf();
    test_long_log();
    return 0;
}

jgb_api_t test_log
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test log",
    .init = init,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = nullptr
};
