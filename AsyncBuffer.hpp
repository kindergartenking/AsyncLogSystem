#pragma once
#include <string>

#include <iostream>
#include <boost/asio.hpp>
using namespace std;
using boost::asio::ip::tcp;



class LogBuffer
{
public:
	LogBuffer() : write_pos_(0), read_pos_(0) {
		buffer_.resize(2048);
	}
    void Push(const char* data, size_t len)
    {
        ToBeEnough(len); // 确保容量足够
        // 开始写入
        std::copy(data, data + len, &buffer_[write_pos_]);
        write_pos_ += len;
    }
    char* ReadBegin(int len)
    {
        assert(len <= ReadableSize());
        return &buffer_[read_pos_];
    }
    bool IsEmpty() { return write_pos_ == read_pos_; }

    void Swap(LogBuffer& buf)
    {
        buffer_.swap(buf.buffer_);
        std::swap(read_pos_, buf.read_pos_);
        std::swap(write_pos_, buf.write_pos_);
    }
    size_t WriteableSize()
    { // 写空间剩余容量
        return buffer_.size() - write_pos_;
    }
    size_t ReadableSize()
    { // 读空间剩余容量
        return write_pos_ - read_pos_;
    }
    const char* Begin() { return &buffer_[read_pos_]; }
    void MoveWritePos(int len)
    {
        assert(len <= WriteableSize());
        write_pos_ += len;
    }
    void MoveReadPos(int len)
    {
        assert(len <= ReadableSize());
        read_pos_ += len;
    }
    void Reset()
    { // 重置缓冲区
        write_pos_ = 0;
        read_pos_ = 0;
    }

protected:
    void ToBeEnough(size_t len)
    {
        int buffersize = buffer_.size();
        if (len >= WriteableSize())
        {
            if (buffer_.size() < 2048)
            {
                buffer_.resize(2 * buffer_.size());
            }
            else
            {
                buffer_.resize( buffersize+2048);
            }
        }
    }

	std::vector<char> buffer_; // 缓冲区
	size_t write_pos_;         // 生产者此时的位置
	size_t read_pos_;
};