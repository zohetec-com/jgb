#include <jgb/core.h>
#include <jgb/helper.h>
#include <jgb/buffer.h>

static jgb::buffer* buf = nullptr;
static jgb::writer* wr = nullptr;
static jgb::reader* rd[2] = {nullptr, nullptr};
static uint8_t data[1024];
static int data_len = 1024;

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
    r = rd[w->id_]->get(&frm);
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

static void tsk_exit(void* worker)
{
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
}

static loop_ptr_t loops[] = { tsk_read, tsk_read, tsk_write, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_buffer_app
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test buffer",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
