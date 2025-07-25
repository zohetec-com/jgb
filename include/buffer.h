#ifndef BUFFER_H
#define BUFFER_H

#include <inttypes.h>
#include <string>
#include <list>
#include <memory>

namespace jgb
{
struct frame
{
    uint8_t* buf; // payload 开始地址
    int len; // payload 长度
    int start_offset; // payload 开始位置相对帧开始位置的偏移量。
    // start_offset + len 要向上对齐到 4 的整数倍。
    //int end_offset; // payload 结束位置相对帧结束位置的偏移量。 目前看不出需要这个。
};

class buffer;

class reader
{
public:
    reader();
    ~reader();

    // 请求从缓冲区获取一帧数据。
    int request_frame(struct frame* frm, int timeout = 100);
    // 释放已请求获取的数据。
    void release();

public:
    int64_t stat_bytes_read_;
    int64_t stat_frames_read_;

    buffer* buf_;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class writer
{
public:
    writer();
    ~writer();

    int request_buffer(uint8_t** buf, int len, int timeout = 100);
    // len 为 0 表示取消。
    int commit(int len, int start_offset = 0);
    int put(uint8_t* buf, int len, int timeout = 100);

public:
    int64_t stat_bytes_written_;
    int64_t stat_frames_written_;

    buffer* buf_;

    // 对应一个缓冲区对象，只有一个 writer 对象。
    int ref_;

    uint8_t* cur_;
    int reserved_;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class buffer
{
public:
    buffer(const std::string& id);
    ~buffer();

    int resize(int len);

    reader* add_reader();
    writer* add_writer();

    int remove_reader(reader* r);
    int remove_writer(writer* w);

public:
    std::string id_;
    int len_;

    uint8_t* start_;
    uint8_t* end_;

    std::list<reader*> readers_;
    std::list<writer*> writers_;

    int ref;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class buffer_manager
{
public:
    static buffer_manager* get_instance();

    buffer* add_buffer(const std::string& id);
    int remove_buffer(buffer* buf);

public:
    std::list<buffer*> buffers_;

private:
    buffer_manager();
    ~buffer_manager();

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

}

#endif // BUFFER_H
