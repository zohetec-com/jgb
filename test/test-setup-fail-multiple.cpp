#include <jgb/core.h>
#include <jgb/helper.h>

class test_setup_fail_multiple_count
{
public:
    test_setup_fail_multiple_count()
        : setup_(0),
        looped_(0)
    {
    }
    ~test_setup_fail_multiple_count()
    {
        jgb_function();
        jgb_assert(setup_ == 1);
        jgb_assert(!looped_);
    }
    int setup_;
    int looped_;
};

static test_setup_fail_multiple_count count;

static int tsk_init(void*)
{
    ++ count.setup_;
    return JGB_ERR_FAIL;
}

static int tsk_loop(void*)
{
    ++ count.looped_;
    jgb_assert(0);
    return 0;
}

static void tsk_exit(void*)
{
    jgb_assert(0);
}

static loop_ptr_t loops[] = { tsk_loop, tsk_loop, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_setup_fail_multiple
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "setup fail multiple",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
