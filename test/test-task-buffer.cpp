#include <jgb/core.h>
#include <jgb/helper.h>
#include <jgb/buffer.h>
#include "check_u32_context.h"
#include "write_32u_context.h"

struct context
{
    jgb::write_32u_context wr_ctx;
    jgb::check_u32_context chk_ctx[2];
};

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    w->task_->instance_->user_ = new context;
    jgb_assert(w->task_->readers_.size() == 2);
    jgb_assert(w->task_->writers_.size() == 1);
    return 0;
}

static int tsk_read(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context* ctx = (context*) w->task_->instance_->user_;
    jgb::reader* rd = w->task_->readers_[w->id_];
    jgb::frame frm;
    int r;
    r = rd->request_frame(&frm);
    if(!r)
    {
        jgb_assert(frm.buf);
        jgb_assert(frm.len > 0);
        jgb_assert(frm.start_offset == 0);
        jgb_assert(!ctx->chk_ctx[w->id_].check(frm.buf, frm.len));
        rd->release();
    }
    return 0;
}

static int tsk_write(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context* ctx = (context*) w->task_->instance_->user_;
    jgb::writer* wr = w->task_->writers_[0];
    int buf_size = wr->buf_->len_;
    int len = random() % (buf_size);
    if(len > 0)
    {
        int r;
        uint8_t* x_buf;
        r = wr->request_buffer(&x_buf, len);
        if(!r)
        {
            ctx->wr_ctx.fill(x_buf, len);
            wr->commit(len);
        }
    }
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context* ctx = (context*) w->task_->instance_->user_;
    ctx->chk_ctx[0].dump();
    ctx->chk_ctx[1].dump();
    delete ctx;
}

static loop_ptr_t loops[] = { tsk_read, tsk_read, tsk_write, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_task_buffer
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
