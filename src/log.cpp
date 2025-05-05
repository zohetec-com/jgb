#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "log.h"

#define LOG_BUF_SIZE 2048

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
    if (isatty(2))
    {
        fprintf(stderr, "%c%s%s%c[0m", 27, colours[level], line, 27);
    }
    else
    {
        fprintf(stderr, "%s", line);
    }
}

void jgb_log(jgb_log_level level, const char* fname, int lineno, const char *format, ...)
{
    char buf[LOG_BUF_SIZE];
    int len = LOG_BUF_SIZE;
    int off = 0;
    int n;

    if(level != JGB_LOG_RAW)
    {
        struct timeval tv;
        struct tm tm;
        struct tm *ptm = NULL;

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

        pid_t tid = getpid();
        n = snprintf(buf + off, len, "[%6d]", tid);
        if(n > 0)
        {
            off += n;
            len -= n;
        }

        n = snprintf(buf + off, len, "[%s:%d]%s", fname, lineno, log_level_name[level]);
        if(n > 0)
        {
            off += n;
            len -= n;
        }
    }

    va_list args;
    va_start(args, format);
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
        if(level != JGB_LOG_RAW)
        {
            if(buf[off - 1] != '\n')
            {
                buf[off++] = '\n';
                buf[off] = '\0';
            }
        }
    }
    to_stderr(level, buf);
}
