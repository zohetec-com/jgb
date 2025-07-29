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
#include "buffer.h"
#include "log.h"
#include "error.h"
#include "helper.h"
#include <boost/thread.hpp>

namespace jgb
{

struct __attribute__((packed)) frame_header
{
    uint32_t serial; // 帧序列号，递增
    int len; // payload 长度，如果 len 为 0，指示读者返回到缓冲区的开始位置。
    int start_offset; // payload 开始位置相对帧开始位置的偏移量。
    int unused;

    int total_len()
    {
        return sizeof(struct frame_header) + JGB_ALIGN(start_offset + len, 4);
    }
};

struct buffer::Impl
{
    boost::shared_mutex rw_mutex;
};

struct writer::Impl
{
    boost::shared_mutex rw_mutex;
};

buffer::buffer(const std::string& id)
    : id_(id),
    len_(0),
    start_(nullptr),
    end_(nullptr),
    writers_(nullptr),
    ref_(0),
    serial_(0),
    pimpl_(new Impl())
{
}

buffer::~buffer()
{
    jgb_assert(ref_ == 0);
    if(start_)
    {
        delete[] start_;
    }
}

// 如果读者或者写者已经 get() 成功，怎么可以重新分配内存呢？
int buffer::resize(int len)
{
    if(len_ > 0)
    {
        jgb_warning("不支持重新调整缓冲区大小");
        return JGB_ERR_DENIED;
    }

    // todo： 最小该多少字节呢？
    if(len < 1024)
    {
        len = 1024;
    }

    jgb_assert(!start_);

    boost::unique_lock<boost::shared_mutex> lock(pimpl_->rw_mutex);

    if(len_ == len)
    {
        return 0; // No change needed
    }

    start_ = new uint8_t[len];
    end_ = start_ + len;
    len_ = len;

    if(writers_)
    {
        // TODO: 有必要获得锁吗？
        boost::unique_lock<boost::shared_mutex> wr_lock(writers_->pimpl_->rw_mutex);
        writers_->cur_ = start_;
    }

    return 0; // Success
}

reader* buffer::add_reader()
{
    boost::unique_lock<boost::shared_mutex> lock(pimpl_->rw_mutex);
    reader* rd = new reader(this);
    readers_.push_back(rd);
    return rd;
}

// TODO: 处理多写者。
// get_buffer() 结果只能归属于一个写者，不能被多个写者共享。
writer* buffer::add_writer()
{
    boost::unique_lock<boost::shared_mutex> lock(pimpl_->rw_mutex);
    if(!writers_)
    {
        writers_ = new writer(this);
        writers_->ref_ = 1;
    }
    else
    {
        ++ writers_->ref_;
    }
    return writers_;
}

int buffer::remove_reader(reader* r)
{
    boost::unique_lock<boost::shared_mutex> lock(pimpl_->rw_mutex);
    for(auto it = readers_.begin(); it != readers_.end(); ++it)
    {
        if(*it == r)
        {
            jgb_assert((*it)->buf_ == this);
            delete *it;
            readers_.erase(it);
            return 0; // 成功
        }
    }
    return -1; // 读者未找到
}

int buffer::remove_writer(writer* w)
{
    boost::unique_lock<boost::shared_mutex> lock(pimpl_->rw_mutex);
    if(writers_ == w)
    {
        jgb_assert(writers_->ref_ > 0);
        -- writers_->ref_;
        if(writers_->ref_ <= 0)
        {
            delete writers_;
            writers_ = nullptr;
        }
        return 0; // 成功
    }
    return -1; // 写者未找到
}

struct buffer_manager::Impl
{
    boost::shared_mutex rw_mutex;
};

buffer_manager* buffer_manager::get_instance()
{
    static buffer_manager instance;
    return &instance;
}

buffer_manager::buffer_manager()
    : pimpl_(new Impl())
{
}

buffer* buffer_manager::add_buffer(const std::string& id)
{
    if(id.empty())
    {
        return nullptr;
    }

    boost::unique_lock<boost::shared_mutex> lock(pimpl_->rw_mutex);

    for(auto& i: buffers_)
    {
        if(i->id_ == id)
        {
            ++ i->ref_;
            return i;
        }
    }

    buffer* buf = new buffer(id);
    buf->ref_ = 1;
    buffers_.push_back(buf);
    return buf;
}

int buffer_manager::remove_buffer(buffer* buf)
{
    boost::unique_lock<boost::shared_mutex> lock(pimpl_->rw_mutex);
    for(auto it = buffers_.begin(); it != buffers_.end(); ++it)
    {
        if(*it == buf)
        {
            jgb_assert((*it)->id_ == buf->id_);
            jgb_assert(buf->ref_ > 0);
            -- buf->ref_;
            if(buf->ref_ <= 0)
            {
                delete buf;
                buffers_.erase(it);
            }
            return 0;
        }
    }
    return -1; // Buffer not found
}

struct reader::Impl
{
    boost::mutex mutex;
    boost::condition_variable wr_commit_cond;
    boost::condition_variable rd_release_cond;
};

reader::reader(buffer *buf)
    : stat_bytes_read_(0L),
    stat_frames_read_(0L),
    buf_(buf),
    cur_(nullptr),
    stored_(0),
    serial_(0),
    holding_(false),
    pimpl_(new Impl())
{
}

int reader::request_frame_internal(struct frame* frm, int timeout)
{
    if(!frm)
    {
        return JGB_ERR_INVALID;
    }

    if(timeout < 0)
    {
        timeout = 0;
    }

    boost::unique_lock<boost::mutex> rd_lock(pimpl_->mutex);
    if(!stored_)
    {
        if(pimpl_->wr_commit_cond.wait_for(rd_lock, boost::chrono::milliseconds(timeout),
                                            [this](){ return stored_ > 0; }))
        {
        }
        else
        {
            ++ stat_timeout_;
            jgb_assert(!stored_);
            return JGB_ERR_TIMEOUT; // 超时
        }
    }

    jgb_assert(cur_);
    jgb_assert(stored_ > 0);

    struct frame_header* hdr = reinterpret_cast<struct frame_header*>(cur_);
    //jgb_debug("{ hdr->serial = %d, serial_ = %d }", hdr->serial, serial_);
    jgb_assert(hdr->serial == serial_);
    if(!hdr->len)
    {
        jgb_assert(!hdr->start_offset);

        // 读者需要返回到缓冲区的开始位置。
        cur_ = buf_->start_;

        -- stored_;
        ++ serial_;

        // 通知写者，读者已经移动读指针。
        rd_lock.unlock();
        pimpl_->rd_release_cond.notify_one();

        ++ stat_frames_read_;

        return JGB_ERR_RETRY;
    }

    frm->buf = cur_ + sizeof(struct frame_header) + hdr->start_offset;
    frm->start_offset = hdr->start_offset;
    frm->len = hdr->len;

    holding_ = true;

    return 0; // 成功
}

int reader::request_frame(struct frame* frm, int timeout)
{
    int r;
    r = request_frame_internal(frm, timeout);
    if(r == JGB_ERR_RETRY)
    {
        r = request_frame_internal(frm, timeout);
    }
    return r;
}

void reader::release()
{
    boost::unique_lock<boost::mutex> rd_lock(pimpl_->mutex);

    //jgb_debug("{ stored = %d, serial = %d }", stored_, serial_);
    if(stored_ > 0)
    {
        -- stored_;
        ++ serial_;

        holding_ = false;

        jgb_assert(cur_);
        jgb_assert(cur_ + sizeof(struct frame_header) <= buf_->end_);

        struct frame_header* hdr = reinterpret_cast<struct frame_header*>(cur_);
        if(hdr->len)
        {
            cur_ += hdr->total_len();
            if(cur_ + sizeof(struct frame_header) > buf_->end_)
            {
                //jgb_debug("reader return");
                cur_ = buf_->start_;
            }
        }
        else
        {
            //jgb_debug("reader return");
            jgb_assert(!hdr->start_offset);
            cur_ = buf_->start_;
        }

        // 通知写者，读指针已经移动。
        rd_lock.unlock();
        pimpl_->rd_release_cond.notify_one();

        stat_bytes_read_ += hdr->len;
        ++ stat_frames_read_;
    }
}

writer::writer(buffer* buf)
    : stat_bytes_written_(0L),
    stat_frames_written_(0L),
    buf_(buf),
    cur_(buf->start_),
    ref_(0),
    requested_len_(0),
    reserved_len_(0),
    pimpl_(new Impl())
{
    jgb_assert(buf_);
}

static int wait_reader_release(writer* wr, boost::unique_lock<boost::mutex>& lock, reader* rd, int timeout)
{
    if(rd->pimpl_->rd_release_cond.wait_for(lock, boost::chrono::milliseconds(timeout)) == boost::cv_status::no_timeout)
    {
        return 0; // 成功
    }
    else
    {
        ++ wr->stat_timeout_;
        return JGB_ERR_TIMEOUT; // 超时
    }
}

// 等待单个读者移动到适当的位置。
// 场景1：从写指针到缓冲区末尾的空间足以容纳新帧。
int writer::wait_reader_scenario_1(reader* rd, int timeout)
{
    boost::unique_lock<boost::mutex> rd_lock(rd->pimpl_->mutex);
    uint8_t* next;
    int r;
    next = cur_ + reserved_len_;
    while(true)
    {
        if(rd->cur_)
        {
            if(cur_ < rd->cur_)
            {
                if(next <= rd->cur_)
                {
                    return 0;
                }
                else
                {
                    r = wait_reader_release(this, rd_lock, rd, timeout);
                    if(r)
                    {
                        return r;
                    }
                }
            }
            else if(cur_ > rd->cur_)
            {
                return 0;
            }
            else
            {
                // 满
                if(rd->stored_)
                {
                    r = wait_reader_release(this, rd_lock, rd, timeout);
                    if(r)
                    {
                        return r;
                    }
                }
                // 空
                else
                {
                    return 0;
                }
            }
        }
        else
        {
            // 读者尚未完成初始化。
            return 0;
        }
    }
}

int writer::wait_readers_scenario_1(int timeout)
{
    int r;
    for(auto& reader : buf_->readers_)
    {
        r = wait_reader_scenario_1(reader, timeout);
        if(r)
        {
            return r;
        }
    }
    return 0;
}

int writer::wait_reader_scenario_2(reader* rd, int timeout)
{
    boost::unique_lock<boost::mutex> rd_lock(rd->pimpl_->mutex);
    uint8_t* next;
    int r;
    next = buf_->start_ + reserved_len_;
    while(true)
    {
        if(rd->cur_)
        {
            if(cur_ < rd->cur_)
            {
                r = wait_reader_release(this, rd_lock, rd, timeout);
                if(r)
                {
                    return r;
                }
            }
            else if(cur_ > rd->cur_)
            {
                if(next <= rd->cur_)
                {
                    return 0;
                }
                else
                {
                    r = wait_reader_release(this, rd_lock, rd, timeout);
                    if(r)
                    {
                        return r;
                    }
                }
            }
            else
            {
                // 满
                if(rd->stored_)
                {
                    r = wait_reader_release(this, rd_lock, rd, timeout);
                    if(r)
                    {
                        return r;
                    }
                }
                // 空
                else
                {
                    return 0;
                }
            }
        }
        else
        {
            // 读者尚未完成初始化。
            return 0;
        }
    }
}

int writer::wait_readers_scenario_2(int timeout)
{
    int r;
    for(auto& reader : buf_->readers_)
    {
        r = wait_reader_scenario_2(reader, timeout);
        if(r)
        {
            return r;
        }
    }
    return 0;
}

int writer::wait_reader_scenario_3(reader* rd, int timeout)
{
    boost::unique_lock<boost::mutex> rd_lock(rd->pimpl_->mutex);
    int r;
    while(true)
    {
        if(rd->cur_)
        {
            if(cur_ < rd->cur_)
            {
                r = wait_reader_release(this, rd_lock, rd, timeout);
                if(r)
                {
                    return r;
                }
            }
            else if(cur_ > rd->cur_)
            {
                r = wait_reader_release(this, rd_lock, rd, timeout);
                if(r)
                {
                    return r;
                }
            }
            else
            {
                // 满
                if(rd->stored_)
                {
                    r = wait_reader_release(this, rd_lock, rd, timeout);
                    if(r)
                    {
                        return r;
                    }
                }
                // 空
                else
                {
                    return 0;
                }
            }
        }
        else
        {
            // 读者尚未完成初始化。
            return 0;
        }
    }
}

int writer::wait_readers_scenario_3(int timeout)
{
    int r;
    for(auto& reader : buf_->readers_)
    {
        r = wait_reader_scenario_3(reader, timeout);
        if(r)
        {
            return r;
        }
    }
    return 0;
}

int writer::request_buffer(uint8_t** buf, int len, int timeout)
{
    int frame_len = JGB_ALIGN(len, 4) + sizeof(struct frame_header);
    uint8_t* next;
    int r;

    if(!buf || len <= 0)
    {
        jgb_warning("Invalid arguments. { buf=%p, len=%d }", buf, len);
        return JGB_ERR_INVALID;
    }

    if(frame_len > buf_->len_)
    {
        jgb_warning("缓冲区容量不足。{ len = %d }", len);
        return JGB_ERR_LIMIT;
    }

    // 在提交已申请的缓冲区之前重复请求分配缓冲区。
    if(reserved_len_ > 0)
    {
        jgb_warning("存在未提交的已分配的缓冲区。 { reserved_len_ = %d }", reserved_len_);
        return JGB_ERR_DENIED;
    }

    // 写者尚未完成初始化。
    if(!cur_)
    {
        return JGB_ERR_FAIL;
    }

    if(timeout < 0)
    {
        timeout = 0;
    }

    boost::shared_lock<boost::shared_mutex> buf_lock(buf_->pimpl_->rw_mutex);
    boost::unique_lock<boost::shared_mutex> wr_lock(pimpl_->rw_mutex);

    next = cur_ + frame_len;
    reserved_len_ = frame_len;
    // 场景1：从写指针到缓冲区末尾的空间足以容纳新帧。
    if(next <= buf_->end_)
    {
        r = wait_readers_scenario_1(timeout);
        if(!r)
        {
            *buf = cur_ + sizeof(struct frame_header);
            requested_len_ = len;
            return 0;
        }
        reserved_len_ = 0;
        return r;
    }
    else
    {
        next = buf_->start_ + frame_len;
        // 场景2：从缓冲区开始到写指针的空间足以容纳新帧。
        if(next <= cur_)
        {
            r = wait_readers_scenario_2(timeout);
            if(!r)
            {
                if(cur_ + sizeof(struct frame_header) <= buf_->end_)
                {
                    // 填写重定向帧
                    struct frame_header* hdr = reinterpret_cast<struct frame_header*>(cur_);
                    hdr->serial = buf_->serial_ ++;
                    hdr->len = 0;
                    hdr->start_offset = 0;
                    hdr->unused = 0;

                    // TODO：此时需要通知读者吗？
                    ack_readers();
                }

                cur_ = buf_->start_;
                *buf = cur_ + sizeof(struct frame_header);
                requested_len_ = len;
                return 0;
            }
            reserved_len_ = 0;
            return r;
        }
        // 场景3：从缓冲区开始的空间足以容纳新帧，但超过了写指针。
        else
        {
            r = wait_readers_scenario_3(timeout);
            if(!r)
            {
                // 重置所有读者的读取位置。
                for(auto& reader : buf_->readers_)
                {
                    reader->cur_ = buf_->start_;
                }

                cur_ = buf_->start_;
                *buf = cur_ + sizeof(struct frame_header);
                requested_len_ = len;
                return 0;
            }
            reserved_len_ = 0;
            return r;
        }
    }
}

void writer::ack_reader(reader* rd)
{
    boost::unique_lock<boost::mutex> rd_lock(rd->pimpl_->mutex);
    if(!rd->cur_)
    {
        jgb_assert(!rd->stored_);
        rd->cur_ = cur_;
        rd->serial_ = buf_->serial_;
    }
    ++ rd->stored_;
    rd_lock.unlock();
    // TODO：允许设置通知阈值，以减少通知次数。
    rd->pimpl_->wr_commit_cond.notify_one();
}

void writer::ack_readers()
{
    for(auto& reader : buf_->readers_)
    {
        ack_reader(reader);
    }
}

int writer::commit(int len, int start_offset)
{
    boost::shared_lock<boost::shared_mutex> buf_lock(buf_->pimpl_->rw_mutex);
    boost::unique_lock<boost::shared_mutex> wr_lock(pimpl_->rw_mutex);
    if(reserved_len_ > 0)
    {
        if(len > 0
            && start_offset >= 0
            && len + start_offset <= requested_len_)
        {
            //jgb_debug("serial = %d", buf_->serial_);

            struct frame_header* hdr = reinterpret_cast<struct frame_header*>(cur_);
            hdr->serial = buf_->serial_;
            hdr->len = len;
            hdr->start_offset = start_offset;
            hdr->unused = 0;

            // 通知所有读者有新写入帧。
            ack_readers();

            // 因为 ack_readers() 引用 buf_->serial_，所以在 ack_readers() 返回后再更新 buf_->serial。
            ++ buf_->serial_;

            cur_ += reserved_len_;
            jgb_assert(cur_ <= buf_->end_);
            if(cur_ + sizeof(struct frame_header) > buf_->end_)
            {
                //jgb_debug("writer return");
                cur_ = buf_->start_;
            }
            reserved_len_ = 0;

            ++ stat_frames_written_;
            stat_bytes_written_ += len;

            return 0; // 成功
        }
        else if(!len) // 取消提交
        {
            reserved_len_ = 0;
            return 0;
        }
        else
        {
            jgb_warning("Invalid arguments. { len=%d, start_offset=%d, requested_len=%d }", len, start_offset, requested_len_);
            return JGB_ERR_INVALID;
        }
    }
    else
    {
        jgb_warning("没有已申请的缓冲区。");
        return JGB_ERR_INVALID;
    }
}

int writer::put(uint8_t* buf, int len, int timeout)
{
    int r;
    uint8_t* x_buf;
    r = request_buffer(&x_buf, len, timeout);
    if(!r)
    {
        memcpy(x_buf, buf, len);
        r = commit(len, 0);
        if(!r)
        {
            return 0;
        }
        else
        {
            jgb_assert(0);
        }
    }
    return r;
}

int writer::fixed_header_size()
{
    return sizeof(struct frame_header);
}

}
