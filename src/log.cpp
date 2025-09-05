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
#include <inttypes.h>

#define LOG_BUF_SIZE 2048

static struct timespec uptime;
int jgb_print_level = JGB_LOG_INFO;

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

static void to_stderr(int level, const char *line)
{
    if(level < jgb_print_level)
    {
        if (isatty(2))
        {
            fprintf(stderr, "%c%s%s%c[0m", 27, colours[level], line, 27);
        }
        else
        {
            fprintf(stderr, "%s", line);
        }
    }
}

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

void jgb_log(jgb_log_level level, const char* fname, int lineno, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if(level == JGB_LOG_RAW)
    {
        if(level < jgb_print_level)
        {
            if (isatty(2))
            {
                fprintf(stderr, "%c%s", 27, colours[level]);
            }
            vfprintf(stderr, format, args);
            if (isatty(2))
            {
                fprintf(stderr, "%c[0m", 27);
            }
        }
        return;
    }

    char buf[LOG_BUF_SIZE];
    int len = LOG_BUF_SIZE;
    int off = 0;
    int n;
    struct timeval tv;
    struct tm tm;
    struct tm *ptm = NULL;

    n = snprintf(buf + off, len, "%s", log_level_name[level]);
    if(n > 0)
    {
        off += n;
        len -= n;
    }

    gettimeofday(&tv, NULL);
    ptm = localtime_r(&tv.tv_sec, &tm);
    if(ptm)
    {
        n = snprintf(buf + off, len,
                     "[%04d/%02d/%02d %02d:%02d:%02d:%03d.%03d]",
                     ptm->tm_year + 1900,
                     ptm->tm_mon + 1,
                     ptm->tm_mday,
                     ptm->tm_hour,
                     ptm->tm_min,
                     ptm->tm_sec,
                     (int) (tv.tv_usec / 1000),
                     (int) (tv.tv_usec % 1000));
        if(n > 0)
        {
            off += n;
            len -= n;
        }
    }

    struct timespec now;
    struct timespec elapse;

    clock_gettime(CLOCK_MONOTONIC, &now);
    timespecsub(&now, &uptime, &elapse);

    long days = elapse.tv_sec / (24 * 3600);
    long hours = (elapse.tv_sec % (24 * 3600)) / 3600;
    long minutes = (elapse.tv_sec % 3600) / 60;
    long seconds = elapse.tv_sec % 60;
    n = snprintf(buf + off, len,
                 "[%ldd %02ld:%02ld:%02ld.%06ld]",
                 days, hours, minutes, seconds, elapse.tv_nsec / 1000);
    if(n > 0)
    {
        off += n;
        len -= n;
    }

    pid_t tid = gettid();
    n = snprintf(buf + off, len, "[%6d]", tid);
    if(n > 0)
    {
        off += n;
        len -= n;
    }

    n = snprintf(buf + off, len, "[%s:%d]", fname, lineno);
    if(n > 0)
    {
        off += n;
        len -= n;
    }

    n = vsnprintf(buf + off, len, format, args);
    if(n > 0)
    {
        off += n;
        len -= n;
    }
    va_end(args);

    if(off > LOG_BUF_SIZE - 3)
    {
        static const char* s = "...[truncated]\n";
        strcpy(buf + LOG_BUF_SIZE - strlen(s) - 1, s);
    }
    else
    {
        if(buf[off - 1] != '\n')
        {
            buf[off++] = '\n';
            buf[off] = '\0';
        }
    }
    to_stderr(level, buf);
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
}
