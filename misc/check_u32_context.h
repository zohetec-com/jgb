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
#ifndef CHECK_U32_CONTEXT_H
#define CHECK_U32_CONTEXT_H

#include <jgb/log.h>
#include <jgb/error.h>
#include <inttypes.h>

namespace jgb
{

class check_u32_context
{
public:
    union
    {
        uint32_t serial_;
        uint8_t bytes[4];
    };

    int pos_;
    bool synced_;

    int64_t stat_check_ok_frames_;
    int64_t stat_check_ok_bytes_;
    int64_t stat_check_fail_;
    int64_t stat_check_fail_frames_;
    int64_t stat_sync_ok_;
    int64_t stat_sync_fail_;
    int64_t stat_sync_skip_;
    int64_t stat_total_;

    check_u32_context()
        : serial_(0),
        pos_(0),
        synced_(false),
        stat_check_ok_frames_(0L),
        stat_check_ok_bytes_(0L),
        stat_check_fail_(0L),
        stat_check_fail_frames_(0L),
        stat_sync_ok_(0L),
        stat_sync_fail_(0L),
        stat_sync_skip_(0L),
        stat_total_(0L)
    {
    }

    void dump()
    {
        jgb_raw("check_u32_context: %p\n", this);
        jgb_raw("stat_check_ok_bytes_ = %lld\n", stat_check_ok_bytes_);
        jgb_raw("stat_check_ok_frames_ = %lld\n", stat_check_ok_frames_);
        jgb_raw("stat_check_fail_ = %lld\n", stat_check_fail_);
        jgb_raw("stat_check_fail_frames_ = %lld\n", stat_check_fail_frames_);
        jgb_raw("stat_sync_ok_ = %lld\n", stat_sync_ok_);
        jgb_raw("stat_sync_fail_ = %lld\n", stat_sync_fail_);
        jgb_raw("stat_sync_skip_ = %lld\n", stat_sync_skip_);
        jgb_raw("stat_total_ = %lld\n", stat_total_);
    }

    int sync(uint8_t* buf, int len)
    {
        uint8_t* p1 = buf;
        uint8_t* p2 = buf + 4;
        uint8_t* p3 = p2 + 4;
        uint8_t* end = buf + len;
        while(p3 <= end)
        {
            uint32_t* p4 = (uint32_t*)p1;
            uint32_t* p5 = (uint32_t*)p2;
            if(*p4 + 1 == *p5)
            {
                synced_ = true;
                pos_ = 0;
                serial_ = *p4;
                ++ stat_sync_ok_;
                return p1 - buf;
            }
            //jgb_dump(p1, 5);
            ++ p1;
            ++ p2;
            ++ p3;
            ++ stat_sync_skip_;
        }
        ++ stat_sync_fail_;
        return -1;
    }

    int check(uint8_t* buf, int len)
    {
        int ret = 0;

        jgb_assert(pos_ < 4);
        jgb_assert(pos_ >= 0);

        if(buf && len > 0)
        {
            stat_total_ += len;

            int off = 0;
            if(!synced_)
            {
                off = sync(buf, len);
                if(off < 0)
                {
                    jgb_fail("sync");
                    ++ stat_check_fail_frames_;
                    return JGB_ERR_FAIL;
                }
            }
            while(off < len)
            {
                if(buf[off] == bytes[pos_])
                {
                    ++ stat_check_ok_bytes_;
                    ++ off;
                    ++ pos_;
                    if(pos_ > 3)
                    {
                        pos_ = 0;
                        ++ serial_;
                    }
                }
                else
                {
                    ret = JGB_ERR_FAIL;
                    ++ stat_check_fail_;
                    jgb_fail("check_u32_context: { [%x] = %02x, expected %02x }",
                              off, buf[off], bytes[pos_]);
                    {
                        int start;
                        int end;

                        start = off - 8;
                        if(start < 0)
                        {
                            start = 0;
                        }

                        end = off + 8;
                        if(end > len)
                        {
                            end = len;
                        }

                        int x_len = end - start;
                        jgb_dump(buf + start, x_len);
                    }

                    int off_off;
                    off_off = sync(buf + off, len - off);
                    jgb_debug("{ off = %d }", off_off);
                    if(off_off < 0)
                    {
                        ++ stat_check_fail_frames_;
                        return JGB_ERR_FAIL;
                    }
                    off += off_off;
                }
            }
        }
        if(!ret)
        {
            ++ stat_check_ok_frames_;
        }
        else
        {
            ++ stat_check_fail_frames_;
        }
        return ret;
    }
};

} // namespace jgb
#endif // CHECK_U32_CONTEXT_H
