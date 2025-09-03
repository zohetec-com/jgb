#include <jgb/core.h>
#include <jgb/helper.h>
#include <jgb/buffer.h>
#include "check_u32_context.h"
#include "write_32u_context.h"

// C++ 不允许同名的 class/struct。
// https://en.wikipedia.org/wiki/One_Definition_Rule
struct context_33129dfc1a36
{
    bool check;
    jgb::check_u32_context chk_ctx;

    context_33129dfc1a36()
    : check(true)
    {
    }
};

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = new context_33129dfc1a36;
    w->task_->instance_->user_ = ctx;
    jgb_assert(w->get_reader(0));
    w->get_config()->get("check", ctx->check);
    jgb_info("{ check = %d }", ctx->check);
    return 0;
}

static int tsk_read(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = (context_33129dfc1a36*) w->task_->instance_->user_;
    jgb::reader* rd = w->get_reader(0);
    jgb::frame frm;
    int r;
    r = rd->request_frame(&frm);
    if(!r)
    {
        jgb_assert(frm.buf);
        jgb_assert(frm.len > 0);
        jgb_assert(frm.start_offset == 0);
        if(ctx->check)
        {
            jgb_assert(!ctx->chk_ctx.check(frm.buf, frm.len));
        }
        rd->release();
    }
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = (context_33129dfc1a36*) w->task_->instance_->user_;
    if(ctx->check)
    {
        ctx->chk_ctx.dump();
    }
    delete ctx;
}

static loop_ptr_t loops[] = { tsk_read, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t read_buffer
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test task buffer",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
