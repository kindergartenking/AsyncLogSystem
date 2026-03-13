#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "AsyncBuffer.hpp"






using functor = std::function<void(LogBuffer&)>;
class AsyncWorker
{
public:
    using ptr = std::shared_ptr<AsyncWorker>;
    //RealPush会传到这里来，线程执行的 callback_(buffer_consumer_); 消费者缓冲区回调函数是RealPush
    AsyncWorker(const functor& cb)
        : 
        callback_(cb),
        stop_(false),
        thread_(std::thread(&AsyncWorker::ThreadEntry, this)) {


    }
    ~AsyncWorker() { Stop(); }
    //主线程写入日志（Push 方法）
    void Push(const char* data, size_t len)
    {
        // 如果生产者队列不足以写下len长度数据，并且缓冲区是固定大小，那么阻塞
        std::unique_lock<std::mutex> lock(mtx_);
            cond_productor_.wait(lock, [&]()
                { return len <= buffer_productor_.WriteableSize(); });
        //// 写入数据到生产者缓冲区
        buffer_productor_.Push(data, len);
        // 唤醒消费者线程处理数据
        cond_consumer_.notify_one();
    }
    void Stop()
    {
        stop_ = true;
        cond_consumer_.notify_all(); // 所有线程把缓冲区内数据处理完就结束了
        thread_.join();
    }

private:
    void ThreadEntry()
    {
        while (!stop_)
        {
            { // 缓冲区交换完就解锁，让productor继续写入数据
                std::unique_lock<std::mutex> lock(mtx_);
                cond_consumer_.wait(lock, [&]()
                    { return stop_ || !buffer_productor_.IsEmpty(); });
                //// 核心：交换两个缓冲区（生产者缓冲区 → 消费者缓冲区）
                buffer_productor_.Swap(buffer_consumer_);
                // 固定容量的缓冲区才需要唤醒
                // 唤醒可能阻塞的生产者（缓冲区有空间了）
                cond_productor_.notify_one();
            }
            callback_(buffer_consumer_); // 调用回调函数对缓冲区中数据进行处理
            buffer_consumer_.Reset();// 恢复buffer_consumer_的可用空间
            if (stop_ && buffer_productor_.IsEmpty())
                return;
        }
    }

private:
   
    std::atomic<bool> stop_; // 用于控制异步工作器的启动
    std::mutex mtx_;
    LogBuffer buffer_productor_;
    LogBuffer buffer_consumer_;
    std::condition_variable cond_productor_;
    std::condition_variable cond_consumer_;
    functor callback_; // 回调函数，用来告知工作器如何落地
    std::thread thread_;
};