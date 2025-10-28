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
    bool is_text;
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

    FILE* fp;

    context_33129dfc1a36()
        : dump(false),
        is_text(false),
        check(true),
        assert_on_error(false),
        sleep_ms(0),
        interval(10),
        stat_recv_bytes(0L),
        stat_recv_frames(0L),
        last_stat_recv_bytes(0L),
        last_stat_recv_frames(0L),
        fp(nullptr)
    {
        last_stat_time =  (struct timespec) {0,0};
    }

    ~context_33129dfc1a36()
    {
        if(fp)
        {
            fclose(fp);
        }
    }
};

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = new context_33129dfc1a36;
    std::string filename;
    w->task_->instance_->user_ = ctx;
    jgb_assert(w->get_reader(0));
    w->get_config()->get("output_file", filename);
    if(!filename.empty())
    {
        ctx->fp = fopen(filename.c_str(), "wb");
        if(!ctx->fp)
        {
            jgb_fail("fopen. { file %s, error %s }", filename.c_str(), strerror(errno));
            delete ctx;
            return JGB_ERR_IO;
        }
    }
    w->get_config()->get("dump", ctx->dump);
    w->get_config()->get("is_text", ctx->is_text);
    w->get_config()->get("check", ctx->check);
    w->get_config()->get("assert_on_error", ctx->assert_on_error);
    w->get_config()->get("sleep_ms", ctx->sleep_ms);
    w->get_config()->get("interval", ctx->interval);
    jgb_info("{ check = %d, assert_on_error = %d, report interval = %d secs }",
             ctx->check, ctx->assert_on_error, ctx->interval);
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
            if(!ctx->is_text)
            {
                jgb_raw("buf id: %s\n", rd->buf_->id().c_str());
                jgb_dump(frm.buf, frm.len);
            }
            else
            {
                jgb_raw("%.*s", frm.len, frm.buf);
            }
        }
        if(ctx->check)
        {
            r = ctx->chk_ctx.check(frm.buf, frm.len);
            if(r && ctx->assert_on_error)
            {
                jgb_assert(0);
            }
        }
        if(ctx->fp)
        {
            size_t n;
            n = fwrite(frm.buf, 1, frm.len, ctx->fp);
            jgb_assert(n == (uint32_t) frm.len);
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

static std::string string_format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return std::string(buf);
}

static const char* quantity_unit(int order)
{
    switch(order)
    {
        case 0: return "B";
        case 1: return "kB";
        case 2: return "MB";
        case 3: return "GB";
        case 4: return "TB";
        default: return "B";
    }
}

static const char* rate_unit(int order)
{
    switch(order)
    {
    case 0: return "B/s";
    case 1: return "kB/s";
    case 2: return "MB/s";
    case 3: return "GB/s";
    case 4: return "TB/s";
    default: return "B/s";
    }
}

static void quantity_order(double v, int& order, double& order_v)
{
    if(v < 1000.0)
    {
        order = 0;
        order_v = v;
        return;
    }
    else if(v < 1000.0 * 1000.0)
    {
        order = 1;
        order_v = v / 1000.0;
        return;
    }
    else if(v < 1000.0 * 1000.0 * 1000.0)
    {
        order = 2;
        order_v = v / (1000.0 * 1000.0);
        return;
    }
    else if(v < 1000.0 * 1000.0 * 1000.0 * 1000.0)
    {
        order = 3;
        order_v = v / (1000.0 * 1000.0 * 1000.0);
        return;
    }
    else
    {
        order = 4;
        order_v = v / (1000.0 * 1000.0 * 1000.0 * 1000.0);
        return;
    }
}

static std::string rate_to_string(double rate)
{
    int order = 0;
    double order_v = rate;
    quantity_order(rate, order, order_v);
    return string_format("%8.2f %s", order_v, rate_unit(order));
}

static std::string quantity_to_string(double quantity)
{
    int order = 0;
    double order_v = quantity;
    quantity_order(quantity, order, order_v);
    return string_format("%8.2f %s", order_v, quantity_unit(order));
}

static int tsk_report(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_33129dfc1a36* ctx = (context_33129dfc1a36*) w->get_user();
    struct timespec now;

    if(!ctx->interval)
    {
        return JGB_ERR_END;
    }

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
        jgb_info("buf %s, reader %s, input %s, %s, %8ld frames, %8.2f fps",
                 w->get_reader(0)->buf_->id().c_str(),
                 w->get_reader(0)->id_.c_str(),
                 quantity_to_string(bytes).c_str(),
                 rate_to_string(rate).c_str(),
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
