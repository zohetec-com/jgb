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
    int reserved;

    int total_len()
    {
        return sizeof(struct frame_header) + JGB_ALIGN(start_offset + len, 4);
    }
};

struct buffer::Impl
{
    boost::shared_mutex rw_mutex;
};

buffer::buffer(const std::string& id)
    : id_(id),
    len_(0),
    start_(nullptr),
    end_(nullptr),
    ref(0),
    pimpl_(new Impl())
{
}

buffer::~buffer()
{
    jgb_assert(ref == 0);
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

    return 0; // Success
}

struct buffer_manager::Impl
{
    boost::shared_mutex rw_mutex;
};

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
            ++ i->ref;
            return i;
        }
    }

    buffer* buf = new buffer(id);
    buf->ref = 1;
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
            if(--buf->ref <= 0)
            {
                delete buf;
            }
            buffers_.erase(it);
            return 0;
        }
    }
    return -1; // Buffer not found
}

int writer::request_buffer(uint8_t** buf, int len, int timeout)
{
    int frame_len = JGB_ALIGN(len, 4) + sizeof(struct frame_header);
    uint8_t* next;

    jgb_assert(buf_);
    jgb_assert(cur_);

    next = cur_ + frame_len;
    if(next <= buf_->end_)
    {
        for(auto& r: buf_->readers_)
        {
            while

        }
    }
    else
    {
        next = buf_->start_ + frame_len;
        if(next <= cur_)
        {

        }
        else
        {

        }
    }

}

}
