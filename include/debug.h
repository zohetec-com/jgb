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
#ifndef DEBUG_H_20250301
#define DEBUG_H_20250301

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define jgb_debug(fmt, ...)     fprintf(stderr, "[%s:%d][DEBUG] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_info(fmt, ...)      fprintf(stderr, "[%s:%d][INFO] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_notice(fmt, ...)    fprintf(stderr, "[%s:%d][NOTICE] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_warning(fmt, ...)   fprintf(stderr, "[%s:%d][WARNING] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_error(fmt, ...)     fprintf(stderr, "[%s:%d][ERROR] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
// 错误的特例
#define jgb_bug(fmt, ...)       fprintf(stderr, "[%s:%d][BUG] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_fail(fmt, ...)      fprintf(stderr, "[%s:%d][FAIL] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_fatal(fmt, ...)     fprintf(stderr, "[%s:%d][FATAL] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define jgb_assert(x)           assert(x)

#define jgb_safe_delete_object(p)       do{ delete p; p=NULL; } while(0)
#define jgb_safe_delete_array(p)        do{ delete[] p; p=NULL; } while(0)
#define jgb_safe_free(p)                do{ if(p){ free(p); p=NULL; } } while(0)

#endif // DEBUG_H
