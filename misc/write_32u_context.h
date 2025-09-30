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
#ifndef WRITE_32U_CONTEXT_H
#define WRITE_32U_CONTEXT_H

#include <jgb/log.h>
#include <inttypes.h>

namespace jgb
{

class write_32u_context
{
public:

    union
    {
        uint32_t serial_;
        uint8_t bytes[4];
    };
    int pos_;

    write_32u_context()
    : serial_(0),
        pos_(0),
        stat_write_frames_(0L),
        stat_write_bytes_(0L)
    {
    }

    void fill(uint8_t* buf, int len)
    {
        jgb_assert(pos_ < 4);
        jgb_assert(pos_ >= 0);

        if(buf)
        {
            int to_fill = len;
            int off = 0;
            while(to_fill > 0)
            {
                buf[off] = bytes[pos_];
                ++ off;
                ++ pos_;
                -- to_fill;
                if(pos_ > 3)
                {
                    pos_ = 0;
                    ++ serial_;
                }
            }
            ++ stat_write_frames_;
            stat_write_bytes_ += len;
        }
    }

    void dump()
    {
        jgb_raw("write_32u_context: %p\n", this);
        jgb_raw("stat_write_frames_ = %lld\n", stat_write_frames_);
        jgb_raw("stat_write_bytes_ = %lld\n", stat_write_bytes_);
    }

    int64_t stat_write_frames_;
    int64_t stat_write_bytes_;
};

} // namespace jgb
#endif // WRITE_32U_CONTEXT_H
