#include "core.h"
#include "buffer.h"
#include <stdio.h>

extern void (*jgb_log_to_other)(int level, const char* preamble, const char *format, va_list ap);

#define LOG_SIZE 1024
int jgb_log_buf_level = JGB_LOG_INFO;
static jgb::buffer* buf = nullptr;
static jgb::writer* wr = nullptr;
static jgb::reader* rd = nullptr;
static int stat_log_fail = 0;

static int to_buf_lower(int len, int level, const char* preamble, int preamble_len, const char *format, va_list ap)
{
    int r;
    log_frame_header* h;

    jgb_assert(wr);
    jgb_assert(buf);
    jgb_assert(buf->len_ > len);

    r = wr->request_buffer((uint8_t**) &h, len, 0);
    if(!r)
    {
        int n;
        int remain = len - sizeof(log_frame_header) - preamble_len;
        jgb_assert(len > (int) sizeof(log_frame_header) + preamble_len);
        n = vsnprintf((char*) (h->log + preamble_len), remain, format, ap);
        //fprintf(stderr, "len = %d, n = %d, preamble len = %d\n", len, n, preamble_len);
        if(n > 0)
        {
            if(n < remain)
            {
                h->level = level;
                h->unused[0] = 0;
                h->unused[1] = 0;
                h->unused[2] = 0;
                memcpy(h->log, preamble, preamble_len);
                if(level != JGB_LOG_RAW)
                {
                    *((char*) (h->log + preamble_len + n)) = '\n';
                    ++ n;
                }
                r = wr->commit(sizeof(log_frame_header) + preamble_len + n);
                jgb_assert(!r);
            }
            else
            {
                r = wr->cancel();
                jgb_assert(!r);
            }
            return sizeof(log_frame_header) + preamble_len + n;
        }
        else
        {
            r = wr->cancel();
            jgb_assert(!r);
        }
    }
    ++ stat_log_fail;
    return -1;
}

static void to_buf(int level, const char* preamble, const char *format, va_list ap)
{
    if(level < jgb_log_buf_level && wr)
    {
        int r;
        va_list ap_copy;
        int n;
        int preamble_len = strlen(preamble);

        va_copy(ap_copy, ap);
        n = to_buf_lower(LOG_SIZE, level, preamble, preamble_len, format, ap);
        if(n > LOG_SIZE)
        {
            r = to_buf_lower(LOG_SIZE, level, preamble, preamble_len, format, ap_copy);
            jgb_assert(r == n);
        }
        va_end(ap_copy);
    }
}

static int init(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    int r;
    std::string buf_id;
    r = c->get("buf_id", buf_id);
    if(!r)
    {
        int buf_size;
        r = c->get("buf_size", buf_size);
        if(!r)
        {
            buf = jgb::buffer_manager::get_instance()->add_buffer(buf_id);
            jgb_assert(buf);
            buf->resize(buf_size);
            wr = buf->add_writer();
            if(wr)
            {
                jgb_log_to_other = to_buf;
            }
            else
            {
                jgb_assert(0);
            }
            rd = buf->add_reader(true);
            jgb_assert(rd);
        }
    }
    c->get("log_buf_level", jgb_log_buf_level);
    c->create("stat_log_fail", 0);
    c->bind("stat_log_fail", &stat_log_fail);
    jgb_log_init();
    return 0;
}

static void release(void*)
{
    if(buf)
    {
        buf->remove_reader(rd);
        buf->remove_writer(wr);
        jgb::buffer_manager::get_instance()->remove_buffer(buf);
        rd = nullptr;
        wr = nullptr;
    }
}

jgb_api_t logbuf
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "log app",
    .init = init,
    .release = release,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = nullptr
};
