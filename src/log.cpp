/*
 * jgb - a framework for linux media streaming application
 *
 * Copyright (C) 2025 Beijing Zohetec Co., Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "helper.h"
#include "buffer.h"
#include <inttypes.h>
#include <boost/thread.hpp>

#define LOG_BUF_SIZE 1024

int jgb_print_level = JGB_LOG_INFO;
static struct timespec uptime;
static jgb::buffer* buf = nullptr;
static jgb::writer* wr = nullptr;
static jgb::reader* rd = nullptr;
static boost::shared_mutex mutex;

static const char* log_level_name [] =
{
    "E:",
    "W:",
    "N:",
    "I:",
    "D:",
    "E:"
};

// ANSI escape codes
// https://gist.github.com/dominikwilkowski/60eed2ea722183769d586c76f22098dd
static const char * const colours[] = {
    "[31;1m", /* ERR */
    "[33;1m", /* WARN */
    "[35;1m", /* NOTICE */
    "[36;1m", /* INFO */
    "[34;1m", /* DEBUG */
    "[33;1m", /* RAW */
    "[33m",
    "[33m",
    "[33m",
    "[32;1m",
    "[0;1m",
    "[31m",
};

static void timespecsub(struct timespec *a, struct timespec *b, struct timespec *res)
{
    res->tv_nsec = a->tv_nsec - b->tv_nsec;
    res->tv_sec = a->tv_sec - b->tv_sec;

    // Handle borrowing if nanoseconds become negative
    if (res->tv_nsec < 0) {
        res->tv_sec--; // Decrement seconds
        res->tv_nsec += 1000000000; // Add 1 second (1,000,000,000 nanoseconds) to nanoseconds
    }
}

static void to_stderr(int level, const char* preamble, const char *format, va_list ap)
{
    if(level < jgb_print_level)
    {
        boost::unique_lock<boost::shared_mutex> lock(mutex);
        if (isatty(2))
        {
            fprintf(stderr, "%c%s", 27, colours[level]);
        }
        fprintf(stderr, "%s", preamble);
        vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
        if (isatty(2))
        {
            fprintf(stderr, "%c[0m", 27);
        }
    }
}

static int to_buf(int len, int level, const char* preamble, int preamble_len, const char *format, va_list ap)
{
    int r;
    log_frame_header* h;
    r = wr->request_buffer((uint8_t**) &h, len, 0);
    if(!r)
    {
        int n;
        int remain = len - sizeof(log_frame_header) - preamble_len;
        jgb_assert(len > (int) sizeof(log_frame_header) + preamble_len);
        n = vsnprintf((char*) (h->log + preamble_len), remain, format, ap);
        //fprintf(stderr, "len = %d, n = %d, preamble len = %d\n", len, n, preamble_len);
        if(n >= 0)
        {
            if(n < remain)
            {
                h->level = level;
                h->unused[0] = 0;
                h->unused[1] = 0;
                h->unused[2] = 0;
                memcpy(h->log, preamble, preamble_len);
                *((char*) (h->log + preamble_len + n)) = '\n';
                r = wr->commit(sizeof(log_frame_header) + preamble_len + n + 1);
                jgb_assert(!r);
            }
            else
            {
                jgb_assert(len == LOG_BUF_SIZE);
                r = wr->cancel();
                jgb_assert(!r);
            }
            return sizeof(log_frame_header) + preamble_len + n + 1;
        }
        else
        {
            r = wr->cancel();
            jgb_assert(!r);
            jgb_assert(0);
        }
    }
    else
    {
        // todo: stat error
    }
    return -1;
}

void jgb_log(jgb_log_level level, const char* fname, int lineno, const char *format, ...)
{
    char buf[LOG_BUF_SIZE];
    int off = 0;
    int r;
    va_list args;

    if(level < JGB_LOG_RAW)
    {
        int len = LOG_BUF_SIZE;
        struct timeval tv;
        struct tm tm;
        struct tm *ptm = NULL;

        r = jgb::put_string(buf, len, off, "%s", log_level_name[level]);
        jgb_assert(!r);

        gettimeofday(&tv, NULL);
        ptm = localtime_r(&tv.tv_sec, &tm);
        if(ptm)
        {
            r = jgb::put_string(buf, len, off,
                           "[%04d/%02d/%02d %02d:%02d:%02d:%03d.%03d]",
                           ptm->tm_year + 1900,
                           ptm->tm_mon + 1,
                           ptm->tm_mday,
                           ptm->tm_hour,
                           ptm->tm_min,
                           ptm->tm_sec,
                           (int) (tv.tv_usec / 1000),
                           (int) (tv.tv_usec % 1000));
            jgb_assert(!r);
        }

        struct timespec now;
        struct timespec elapse;

        clock_gettime(CLOCK_MONOTONIC, &now);
        timespecsub(&now, &uptime, &elapse);

        long days = elapse.tv_sec / (24 * 3600);
        long hours = (elapse.tv_sec % (24 * 3600)) / 3600;
        long minutes = (elapse.tv_sec % 3600) / 60;
        long seconds = elapse.tv_sec % 60;
        r = jgb::put_string(buf, len, off,
                       "[%ldd %02ld:%02ld:%02ld.%06ld]",
                       days, hours, minutes, seconds, elapse.tv_nsec / 1000);
        jgb_assert(!r);

        pid_t tid = gettid();
        r = jgb::put_string(buf, len, off, "[%6d]", tid);
        jgb_assert(!r);

        r = jgb::put_string(buf, len, off, "[%s:%d]", fname, lineno);
        jgb_assert(!r);
    }

    va_start(args, format);
    to_stderr(level, buf, format, args);
    va_end(args);

    if(wr)
    {
        int n;
        va_start(args, format);
        n = to_buf(LOG_BUF_SIZE, level, buf, off, format, args);
        va_end(args);
        if(n > LOG_BUF_SIZE)
        {
            va_start(args, format);
            r = to_buf(n, level, buf, off, format, args);
            jgb_assert(r == n);
            va_end(args);
        }
    }
}

void jgb_dump(void* buf, int len)
{
    jgb_raw("buf = %p, len = %d:\n", buf, len);
    for(int i=0; i<len; i++)
    {
        if(!(i%16))
        {
            if(i)
            {
                jgb_raw("\n");
            }
            jgb_raw("%4x: ", i);
        }
        jgb_raw("%02x ", *(((uint8_t*)buf)+i));
    }
    jgb_raw("\n");
}

void jgb_log_init()
{
    clock_gettime(CLOCK_MONOTONIC, &uptime);
    buf = jgb::buffer_manager::get_instance()->add_buffer("jgb#log");
    buf->resize(256*1024);
    wr = buf->add_writer();
    rd = buf->add_reader(true);
}

void jgb_log_fini()
{
    buf->remove_reader(rd);
    buf->remove_writer(wr);
    jgb::buffer_manager::get_instance()->remove_buffer(buf);
    wr = nullptr;
    buf = nullptr;
}
