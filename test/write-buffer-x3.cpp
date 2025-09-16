#include <jgb/core.h>
#include <jgb/helper.h>
#include <jgb/buffer.h>
#include "write_32u_context.h"

// C++ 不允许同名的 class/struct。
// https://en.wikipedia.org/wiki/One_Definition_Rule
struct context_067655987bb2
{
    jgb::write_32u_context wr_ctx;
    int fixed_len;

    context_067655987bb2()
    : fixed_len(0)
    {
    }
};

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_067655987bb2* ctx = new context_067655987bb2;
    w->get_config()->get("fixed_len", ctx->fixed_len);
    w->task_->instance_->user_ = ctx;
    jgb_assert(w->get_writer(0));
    return 0;
}

static int tsk_write(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_067655987bb2* ctx = (context_067655987bb2*) w->task_->instance_->user_;
    jgb::writer* wr = w->get_writer(0);
    int buf_size = wr->buf_->len_;
    int len = ctx->fixed_len;

    if(!len)
    {
        len = random() % (buf_size);
    }

    if(len > 0)
    {
        int r;
        uint8_t* x_buf;
        r = wr->request_buffer(&x_buf, len);
        if(!r)
        {
            if(ctx->fixed_len || len % 20)
            {
                ctx->wr_ctx.fill(x_buf, len);
                wr->commit(len);
            }
            else
            {
                wr->cancel();
            }
        }
    }
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_067655987bb2* ctx = (context_067655987bb2*) w->task_->instance_->user_;
    jgb_raw("write buf: %s\n", w->get_writer(0)->buf_->id().c_str());
    ctx->wr_ctx.dump();
    delete ctx;
}

static loop_ptr_t loops[] = { tsk_write, tsk_write, tsk_write, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t write_buffer_x3
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
