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
#ifndef APPCALLBACK_H_20250331
#define APPCALLBACK_H_20250331

typedef int(*loop_ptr_t)(void*);

// TODO: 重命名
typedef struct jgb_loop
{
    int (*setup)(void* worker);
    int (**loops)(void* worker);
    void (*exit)(void* worker);
} jgb_loop_t;

#define MAKE_API_VERSION(major,minor)   ((major<<8)|(minor & 0xFF))
#define CURRENT_API_VERSION()           MAKE_API_VERSION(0, 1)

typedef struct jgb_api
{
    int version;
    const char* desc;
    int (*init)(void* conf); // 初始化应用；conf 是应用的配置
    void (*release)(void* conf); // 释放应用；conf 是应用的配置
    int (*create)(void* conf);  // 创建实例；conf 是实例的配置
    void (*destroy)(void* conf); // 销毁实例；conf 是实例的配置
    int  (*commit)(void* conf); // 使已更改的设置生效；conf 是实例的配置
    jgb_loop_t* loop;
} jgb_api_t;

#endif // APPCALLBACK_H
