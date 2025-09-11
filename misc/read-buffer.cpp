#include <jgb/core.h>
#include <jgb/helper.h>
#include <jgb/buffer.h>
#include "check_u32_context.h"
#include "write_32u_context.h"

// C++ 不允许同名的 class/struct。
// https://en.wikipedia.org/wiki/One_Definition_Rule
struct context_33129dfc1a36
{
    bool dump;
    bool check;
    bool assert_on_error;
    int sleep_ms;
    int interval; // stat report interval, in seconds
    int64_t stat_recv_bytes;
    int64_t stat_recv_frames;
    jgb::check_u32_context chk_ctx;

    struct timespec last_stat_time;
    int64_t last_stat_recv_bytes;
    int64_t last_stat_recv_frames;

    context_33129dfc1a36()
        : dump(false),
        check(true),
        assert_on_error(false),
        sleep_ms(0),
        interval(10),
        stat_recv_bytes(0L),
        stat_recv_frames(0L),
        last_stat_recv_bytes(0L),
        last_stat_recv_frames(0L)
    {
        last_stat_time =  (struct timespec) {0,0};
    }
};

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = new context_33129dfc1a36;
    w->task_->instance_->user_ = ctx;
    jgb_assert(w->get_reader(0));
    w->get_config()->get("dump", ctx->dump);
    w->get_config()->get("check", ctx->check);
    w->get_config()->get("assert_on_error", ctx->assert_on_error);
    w->get_config()->get("sleep_ms", ctx->sleep_ms);
    jgb_info("{ check = %d, assert_on_error = %d }", ctx->check, ctx->assert_on_error);
    return 0;
}

static int tsk_read(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = (context_33129dfc1a36*) w->get_user();
    jgb::reader* rd = w->get_reader(0);
    jgb::frame frm;
    int r;
    r = rd->request_frame(&frm);
    if(!r)
    {
        jgb_assert(frm.buf);
        jgb_assert(frm.len > 0);
        jgb_assert(frm.start_offset == 0);
        if(ctx->dump)
        {
            jgb_dump(frm.buf, frm.len);
        }
        if(ctx->check)
        {
            r = ctx->chk_ctx.check(frm.buf, frm.len);
            if(r && ctx->assert_on_error)
            {
                jgb_assert(0);
            }
        }
        ctx->stat_recv_bytes += frm.len;
        ++ ctx->stat_recv_frames;
        rd->release();
        if(ctx->sleep_ms > 0)
        {
            jgb::sleep(ctx->sleep_ms);
        }
    }
    return 0;
}

static int tsk_report(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = (context_33129dfc1a36*) w->get_user();
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if(ctx->last_stat_time.tv_sec)
    {
        int64_t now_us = now.tv_sec * 1000000 + now.tv_nsec / 1000;
        int64_t last_us = ctx->last_stat_time.tv_sec * 1000000 + ctx->last_stat_time.tv_nsec / 1000;
        int64_t elapse = now_us - last_us;
        int64_t bytes = ctx->stat_recv_bytes - ctx->last_stat_recv_bytes;
        int64_t frames = ctx->stat_recv_frames - ctx->last_stat_recv_frames;
        jgb_assert(bytes >= 0);
        double rate = (double) bytes * 1000000 / (double) elapse;
        double fps = (double) frames * 1000000 / (double) elapse;
        jgb_info("reader %d, input %8ld bytes, %8.2f B/s, %8ld frames, %8.2f fps",
                 w->get_instance()->id_,
                 bytes, rate,
                 frames, fps);
    }
    ctx->last_stat_time = now;
    ctx->last_stat_recv_bytes = ctx->stat_recv_bytes;
    ctx->last_stat_recv_frames = ctx->stat_recv_frames;
    jgb::sleep(ctx->interval * 1000);
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = (context_33129dfc1a36*) w->get_user();
    if(ctx->check)
    {
        jgb_raw("read buf: %s\n", w->get_reader(0)->buf_->id().c_str());
        ctx->chk_ctx.dump();
    }
    delete ctx;
}

static loop_ptr_t loops[] = { tsk_read, tsk_report, nullptr };

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
