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
#ifndef ERROR_H_20250301
#define ERROR_H_20250301

enum jgb_error
{
    JGB_OK = 0,                 // 成功，一切正常
    JGB_ERR_FAIL = 1000,        // 处理请求失败
    JGB_ERR_DENIED,             // 系统拒绝处理该请求
    JGB_ERR_INVALID,            // 无效的请求：内容缺失、格式无效
    JGB_ERR_NOT_SUPPORT,        // 系统不支持处理该请求：尚未实现，或者功能已经被移除
    JGB_ERR_IGNORED,            // 系统忽略处理该请求：没有什么需要做的
    JGB_ERR_NOT_FOUND,          // 没有找到所请求的对象
    JGB_ERR_RETRY               // 请重试：资源忙
};

#endif // ERROR_H
