#include <jgb/core.h>
#include <jgb/helper.h>
#include <jgb/buffer.h>
#include "check_u32_context.h"
#include "write_32u_context.h"

static jgb::write_32u_context* wr_ctx;
static jgb::check_u32_context* chk_ctx[2];
static jgb::buffer* buf = nullptr;
static jgb::writer* wr = nullptr;
static jgb::reader* rd[2] = {nullptr, nullptr};
static uint8_t data[1024];
static int data_len = 1024;

// 最接近缓冲区结束位置的一帧的结束位置与缓冲区的结束位置重合。
static void test_01()
{
    jgb::write_32u_context wr_ctx;
    jgb::check_u32_context chk_ctx;
    jgb::buffer* buf = jgb::buffer_manager::get_instance()->add_buffer("test#01");
    jgb::writer* wr = buf->add_writer();
    jgb::reader* rd = buf->add_reader();
    uint8_t data[1024];
    int r;

    jgb_assert(buf && wr && rd);

    jgb_debug("{ buf = %p, buf->id = %s, buf->ref = %d",
              buf, buf->id_.c_str(), buf->ref_);

    buf->resize(1024);

    for(int i=0; i<30; i++)
    {
        //jgb_debug("{ i = %d }", i);

        wr_ctx.fill(data, 239);
        r = wr->put(data, 239);
        jgb_assert(!r);

        wr_ctx.fill(data, 237);
        r = wr->put(data, 237);
        jgb_assert(!r);

        wr_ctx.fill(data, 495);
        r = wr->put(data, 495);
        jgb_assert(!r);

        r = wr->put(data, 1);
        jgb_assert(r);

        struct jgb::frame frm;
        uint8_t* p = buf->start_;

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 239);
        jgb_assert(frm.start_offset == 0);
        p += jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 237);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(239,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 495);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(237,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(r); // no more data
    }
    buf->remove_reader(rd);
    buf->remove_writer(wr);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

// 写者返回缓冲区开始位置写入，且不等待读者返回缓冲区开始位置。
static void test_02()
{
    jgb::write_32u_context wr_ctx;
    jgb::check_u32_context chk_ctx;
    jgb::buffer* buf = jgb::buffer_manager::get_instance()->add_buffer("test#02");
    jgb::writer* wr = buf->add_writer();
    jgb::reader* rd = buf->add_reader();
    uint8_t data[1024];
    int r;

    jgb_assert(buf && wr && rd);

    jgb_debug("{ buf = %p, buf->id = %s, buf->ref = %d }",
              buf, buf->id_.c_str(), buf->ref_);

    buf->resize(1024);

    for(int i=0; i<30; i++)
    {
        //jgb_debug("{ i = %d }", i);

        wr_ctx.fill(data, 239);

        uint8_t* x_buf;
        r = wr->request_buffer(&x_buf, 239);
        jgb_assert(!r);
        wr->cancel();

        r = wr->put(data, 239);
        jgb_assert(!r);

        wr_ctx.fill(data, 237);
        r = wr->put(data, 237);
        jgb_assert(!r);

        wr_ctx.fill(data, 300);
        r = wr->put(data, 300);
        jgb_assert(!r);

        struct jgb::frame frm;
        uint8_t* p = buf->start_;

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 239);
        jgb_assert(frm.start_offset == 0);
        p += jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 237);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(239,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 300);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(237,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();
    }
    buf->remove_reader(rd);
    buf->remove_writer(wr);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

// 写者返回缓冲区开始位置写入，等待读者返回缓冲区开始位置。
static void test_03()
{
    jgb::write_32u_context wr_ctx;
    jgb::check_u32_context chk_ctx;
    jgb::buffer* buf = jgb::buffer_manager::get_instance()->add_buffer("test#03");
    jgb::writer* wr = buf->add_writer();
    jgb::reader* rd = buf->add_reader();
    uint8_t data[1024];
    int r;

    jgb_assert(buf && wr && rd);

    jgb_debug("{ buf = %p, buf->id = %s, buf->ref = %d }",
              buf, buf->id_.c_str(), buf->ref_);

    buf->resize(1024);

    for(int i=0; i<30; i++)
    {
        //jgb_debug("{ i = %d }", i);

        wr_ctx.fill(data, 239);
        r = wr->put(data, 239);
        jgb_assert(!r);

        wr_ctx.fill(data, 237);
        r = wr->put(data, 237);
        jgb_assert(!r);

        wr_ctx.fill(data, 800);
        r = wr->put(data, 800);
        jgb_assert(r);

        struct jgb::frame frm;
        uint8_t* p = buf->start_;

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 239);
        jgb_assert(frm.start_offset == 0);
        p += jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();

        r = wr->put(data, 800);
        jgb_assert(r);

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 237);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(239,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();

        r = wr->put(data, 800);
        jgb_assert(!r);

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 800);
        jgb_assert(frm.start_offset == 0);
        jgb_assert(frm.buf == buf->start_ + jgb::writer::fixed_header_size());
        jgb_assert(!chk_ctx.check(frm.buf, frm.len));
        rd->release();
    }
    buf->remove_reader(rd);
    buf->remove_writer(wr);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

static void test_check_u32()
{
    jgb::write_32u_context wr_ctx;
    jgb::check_u32_context chk_ctx;
    uint8_t data[1024];

    wr_ctx.serial_ = 0x1000;
    wr_ctx.fill(data, 1024);

    *((uint32_t*)(data+10)) = 0;
    *((uint32_t*)(data+99)) = 0xFFFFFFFFUL;

    jgb_assert(chk_ctx.check(data, 1024));
    chk_ctx.dump();

    wr_ctx.fill(data, 11);
    jgb_assert(!chk_ctx.check(data, 11));

    wr_ctx.fill(data, 10);
    jgb_assert(!chk_ctx.check(data, 10));

    wr_ctx.fill(data, 3);
    jgb_assert(!chk_ctx.check(data, 3));

    chk_ctx.dump();
}

static void test_04()
{
    jgb::write_32u_context wr_ctx;
    jgb::check_u32_context chk_ctx;
    jgb::buffer* buf = jgb::buffer_manager::get_instance()->add_buffer("test#04");
    jgb::writer* wr = buf->add_writer();
    jgb::reader* rd = buf->add_reader();
    buf->resize(10240);
    for(int i=0; i<1000; i++)
    {
        int len = random() % 256;
        uint8_t* p;
        int r = wr->request_buffer(&p, 256);
        if(!r)
        {
            wr_ctx.fill(p, len);
            r = wr->commit(len);
            jgb_assert(!r);
        }

        struct jgb::frame frm;
        r = rd->request_frame(&frm);
        if(!r)
        {
            jgb_assert(frm.start_offset == 0);
            jgb_assert(!chk_ctx.check(frm.buf, frm.len));
            rd->release();
        }
    }
    buf->remove_writer(wr);
    buf->remove_reader(rd);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

// 多个写者
static void test_05()
{
    jgb::write_32u_context wr_ctx;
    jgb::buffer* buf = jgb::buffer_manager::get_instance()->add_buffer("test#05");
    jgb::writer* wr0 = buf->add_writer();
    jgb::writer* wr1 = buf->add_writer();
    int r;
    uint8_t* p;

    buf->resize(10240);
    r = wr0->request_buffer(&p, 1024);
    jgb_assert(!r);
    r = wr1->request_buffer(&p, 1024, 1000);
    jgb_assert(r == JGB_ERR_TIMEOUT);
    r = wr1->commit_all();
    jgb_assert(r == JGB_ERR_INVALID);
    r = wr0->commit_all();
    jgb_assert(!r);
    buf->remove_writer(wr0);
    buf->remove_writer(wr1);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

// 可丢弃的未读数据。
static void test_06()
{
    jgb::buffer* buf = jgb::buffer_manager::get_instance()->add_buffer("test#06");
    jgb::writer* wr = buf->add_writer();
    jgb::reader* rd = buf->add_reader();
    buf->resize(168);
    rd->discard_ = true;
    int r;
    uint8_t* p;
    uint8_t* p1;
    r = wr->request_buffer(&p, 20);
    jgb_assert(!r);
    r = wr->commit_all();
    jgb_assert(!r);
    r = wr->request_buffer(&p, 60);
    jgb_assert(!r);
    r = wr->commit_all();
    jgb_assert(!r);
    r = wr->request_buffer(&p, 40);
    jgb_assert(!r);
    p1 = p;
    r = wr->commit_all();
    jgb_assert(!r);
    jgb_assert(rd->stored_ == 3);
    r = wr->request_buffer(&p, 40);
    jgb_assert(!r);
    jgb_assert(rd->stored_ == 1);
    struct jgb::frame frm;
    r = rd->request_frame(&frm);
    jgb_assert(!r);
    jgb_assert(p1 == frm.buf);
    r = wr->commit_all();
    jgb_assert(!r);
    r = wr->request_buffer(&p, 40);
    jgb_assert(!r);
    r = wr->commit_all();
    jgb_assert(!r);
    r = wr->request_buffer(&p, 40);
    jgb_assert(r);
    rd->release();
    r = wr->request_buffer(&p, 40);
    jgb_assert(!r);
    r = wr->commit_all();
    jgb_assert(!r);
    buf->remove_writer(wr);
    buf->remove_reader(rd);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

static int init(void*)
{
    test_06();
    test_05();
    test_04();
    test_check_u32();
    test_03();
    test_02();
    test_01();
    return 0;
}

static int tsk_init(void*)
{
    wr_ctx = new jgb::write_32u_context();
    chk_ctx[0] = new jgb::check_u32_context();
    chk_ctx[1] = new jgb::check_u32_context();
    buf = jgb::buffer_manager::get_instance()->add_buffer("test#0");
    jgb_assert(buf);
    buf->resize(1024*1024);
    wr = buf->add_writer();
    jgb_assert(wr);
    rd[0] = buf->add_reader();
    jgb_assert(rd[0]);
    rd[1] = buf->add_reader();
    jgb_assert(rd[1]);
    return 0;
}

static int tsk_read(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb::frame frm;
    int r;
    r = rd[w->id_]->request_frame(&frm);
    if(!r)
    {
        jgb_assert(frm.buf);
        jgb_assert(frm.len > 0);
        jgb_assert(frm.start_offset == 0);
        jgb_assert(!chk_ctx[w->id_]->check(frm.buf, frm.len));
        rd[w->id_]->release();
    }
    return 0;
}

static int tsk_write(void*)
{
    int len = random() % data_len;
    if(len > 0)
    {
        int r;
        wr_ctx->fill(data, len);
        r = wr->put(data, len);
        jgb_assert(!r);
        jgb::sleep(1);
    }
    return 0;
}

static void tsk_exit(void*)
{
    chk_ctx[0]->dump();
    chk_ctx[1]->dump();
    delete wr_ctx;
    delete chk_ctx[0];
    delete chk_ctx[1];
    buf->remove_reader(rd[0]);
    buf->remove_reader(rd[1]);
    buf->remove_writer(wr);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

static loop_ptr_t loops[] = { tsk_read, tsk_read, tsk_write, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_buffer
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test buffer",
    .init = init,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
