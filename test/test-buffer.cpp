#include <jgb/core.h>
#include <jgb/helper.h>
#include <jgb/buffer.h>

static jgb::buffer* buf = nullptr;
static jgb::writer* wr = nullptr;
static jgb::reader* rd[2] = {nullptr, nullptr};
static uint8_t data[1024];
static int data_len = 1024;
#if 0
static void fill()
{
    static uint32_t i = 0;
    int n = data_len / sizeof(uint32_t);
    for(int x=0; x<n; x++)
    {
        ((uint32_t*)data)[x] = i++;
    }
}

static void check(int id, const jgb::frame& frm)
{
    static uint32_t i[2] = {0};
    int n = frm.len / sizeof(uint32_t);
    for(int x=0; x<n; x++)
    {
        jgb_assert(((uint32_t*)frm.buf)[x] == i[id]++);
    }
}
#endif

// 最接近缓冲区结束位置的一帧的结束位置与缓冲区的结束位置重合。
static void test_01()
{
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

        r = wr->put(data, 239);
        jgb_assert(!r);

        r = wr->put(data, 237);
        jgb_assert(!r);

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
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 237);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(239,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 495);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(237,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
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

        r = wr->put(data, 239);
        jgb_assert(!r);

        r = wr->put(data, 237);
        jgb_assert(!r);

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
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 237);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(239,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        rd->release();

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 300);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(237,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        rd->release();
    }
    buf->remove_reader(rd);
    buf->remove_writer(wr);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

// 写者返回缓冲区开始位置写入，等待读者返回缓冲区开始位置。
static void test_03()
{
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

        r = wr->put(data, 239);
        jgb_assert(!r);

        r = wr->put(data, 237);
        jgb_assert(!r);

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
        rd->release();

        r = wr->put(data, 800);
        jgb_assert(r);

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 237);
        jgb_assert(frm.start_offset == 0);
        p += JGB_ALIGN(239,4) + jgb::writer::fixed_header_size();
        jgb_assert(frm.buf == p);
        rd->release();

        r = wr->put(data, 800);
        jgb_assert(!r);

        r = rd->request_frame(&frm);
        jgb_assert(!r);
        jgb_assert(frm.len == 800);
        jgb_assert(frm.start_offset == 0);
        jgb_assert(frm.buf == buf->start_ + jgb::writer::fixed_header_size());
        rd->release();
    }
    buf->remove_reader(rd);
    buf->remove_writer(wr);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

static int init(void*)
{
    test_03();
    test_02();
    test_01();
    return 0;
}

static int tsk_init(void*)
{
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
        jgb::sleep(1);
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
        r = wr->put(data, len);
        jgb_assert(!r);
        jgb::sleep(1);
    }
    return 0;
}

static void tsk_exit(void*)
{
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
