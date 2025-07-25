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
    reader(buffer* buf);
    //~reader();

    // 请求从缓冲区获取一帧数据。
    int request_frame(struct frame* frm, int timeout = 100);
    // 释放已请求获取的数据。
    void release();

public:
    int64_t stat_bytes_read_;
    int64_t stat_frames_read_;
    int64_t stat_timeout_;

    buffer* buf_;
    // 读指针。
    uint8_t* cur_;
    // 可读帧数。
    int stored_;
    // 期待读取的帧序号。
    uint32_t serial_;
    // request_frame 成功后持有该帧。
    // 禁止覆盖已经被读者持有的帧。
    bool holding_;

public:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

private:
    int request_frame_internal(struct frame* frm, int timeout);
};

class writer
{
public:
    writer(buffer* buf);
    //~writer();

    int request_buffer(uint8_t** buf, int len, int timeout = 100);
    // len 为载荷数据长度，单位字节; 0 表示取消。
    int commit(int len, int start_offset = 0);
    int put(uint8_t* buf, int len, int timeout = 100);

public:
    int64_t stat_bytes_written_;
    int64_t stat_frames_written_;
    int64_t stat_timeout_;

    buffer* buf_;
    // 写指针。
    uint8_t* cur_;
    // 对应一个缓冲区对象，只有一个 writer 对象。
    int ref_;
    // 所请求的长度。
    int requested_len_;
    // 保留的长度：包括帧头、载荷、填充。
    int reserved_len_;

    static int fixed_header_size();

private:
    int wait_readers_scenario_1(int timeout);
    int wait_reader_scenario_1(reader* rd, int timeout);

    int wait_readers_scenario_2(int timeout);
    int wait_reader_scenario_2(reader* rd, int timeout);

    int wait_readers_scenario_3(int timeout);
    int wait_reader_scenario_3(reader* rd, int timeout);

    void ack_readers();
    void ack_reader(reader* rd);

public:
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
    writer* writers_;

    int ref_;

    // 考虑：如果写者关闭，又打开。
    uint32_t serial_;

public:
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
    //~buffer_manager();

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

}

#endif // BUFFER_H
