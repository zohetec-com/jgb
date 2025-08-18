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
#ifndef LOG_H_20250301
#define LOG_H_20250301

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

enum jgb_log_level
{
    JGB_LOG_ERROR = 0,
    JGB_LOG_WARNING,
    JGB_LOG_NOTICE,
    JGB_LOG_INFO,
    JGB_LOG_DEBUG,
    JGB_LOG_RAW
};

void jgb_log(enum jgb_log_level level, const char* fname, int lineno, const char *format, ...);

#ifdef DEBUG
#define jgb_assert(x)           assert(x)
#else
#define jgb_assert(x)
#endif

#define jgb_debug(fmt, ...)     jgb_log(JGB_LOG_DEBUG,   __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define jgb_info(fmt, ...)      jgb_log(JGB_LOG_INFO,    __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define jgb_notice(fmt, ...)    jgb_log(JGB_LOG_NOTICE,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define jgb_warning(fmt, ...)   jgb_log(JGB_LOG_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define jgb_error(fmt, ...)     jgb_log(JGB_LOG_ERROR,   __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// notice 特例
#define jgb_ok(fmt, ...)        jgb_notice(fmt "   [OK]", ##__VA_ARGS__)

// 调试的特例
#define jgb_function()          jgb_debug("call %s()", __FUNCTION__)
#define jgb_mark()              jgb_debug("\n")

// 错误的特例
#define jgb_bug(fmt, ...) \
    do { \
        jgb_error(fmt "   [BUG]", ##__VA_ARGS__); \
        jgb_assert(0); \
       } while(0)
#define jgb_fail(fmt, ...) \
    jgb_error(fmt "   [FAILED]", ##__VA_ARGS__);
#define jgb_fatal(fmt, ...) \
    do { \
        jgb_error(fmt "   [FATAL]", ##__VA_ARGS__);\
        jgb_assert(0); \
       } while(0)

#define jgb_raw(fmt, ...)       jgb_log(JGB_LOG_RAW, NULL, 0, fmt, ##__VA_ARGS__)

void jgb_dump(void* buf, int len);

void jgb_log_init();

#endif // LOG_H
