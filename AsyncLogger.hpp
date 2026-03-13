#pragma once
#include <atomic>
#include <cassert>
#include <cstdarg>
#include <memory>
#include <mutex>
#include <format> 
#include <stdexcept>
#include "Level.hpp"
#include "AsyncWorker.hpp"
#include "LogFlush.hpp"
#include "AsioThread.hpp"
#include "LogMessage.hpp"
using namespace std;
class AsyncLogger
{
public:
    using ptr = std::shared_ptr<AsyncLogger>;
    AsyncLogger(const std::string& logger_name, std::vector<LogFlush::ptr>& flushs)
        : logger_name_(logger_name),//初始化日志器的名字
        flushs_(flushs.begin(), flushs.end()),//添加实例化方式给日志器，如日志输出到文件还是标准输出，可能有多种
        asyncworker(std::make_shared<AsyncWorker>(//启动异步工作器
            std::bind(&AsyncLogger::RealFlush, this, std::placeholders::_1))) {
    }
    virtual ~AsyncLogger() {};
    std::string Name() { return logger_name_; }
    //该函数则是特定日志级别的日志信息的格式化，当外部调用该日志器时，使用debug模式的日志就会进来
    //在serialize时把日志信息中的日志级别定义为DEBUG。



    template<typename... Args>
    void Debug(const std::string& file, size_t line, const std::string& format, Args&&... args)
    {
        try {
            // C++20 std::format：类型安全、无内存泄漏、支持现代格式化
            std::string log_msg = std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
            serialize(LogLevel::value::DEBUG, file, line, log_msg);
        }
        catch (const std::format_error& e) {
            throw std::runtime_error("Log format error: " + std::string(e.what()));
        }
    };


    template<typename... Args>
    void Info(const std::string& file, size_t line, const std::string& format, Args&&... args)
    {
        try {
            // C++20 std::format：类型安全、无内存泄漏、支持现代格式化
            std::string log_msg = std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
            serialize(LogLevel::value::INFO, file, line, log_msg);
        }
        catch (const std::format_error& e) {
            throw std::runtime_error("Log format error: " + std::string(e.what()));
        }
    };

    template<typename... Args>
    void Warn(const std::string& file, size_t line, const std::string& format, Args&&... args)
    {
        try {
            // C++20 std::format：类型安全、无内存泄漏、支持现代格式化
            std::string log_msg = std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
            serialize(LogLevel::value::WARN, file, line, log_msg);
        }
        catch (const std::format_error& e) {
            throw std::runtime_error("Log format error: " + std::string(e.what()));
        }
    };


    template<typename... Args>
    void Error(const std::string& file, size_t line, const std::string& format, Args&&... args)
    {
        try {
            // c++20 std::format：类型安全、无内存泄漏、支持现代格式化
            std::string log_msg = std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
            serialize(LogLevel::value::Error, file, line, log_msg);
        }
        catch (const std::format_error& e) {
            throw std::runtime_error("log format error: " + std::string(e.what()));
        }
    };

    template<typename... Args>
    void Fatal(const std::string& file, size_t line, const std::string& format, Args&&... args)
    {
        try {
            // C++20 std::format：类型安全、无内存泄漏、支持现代格式化
            std::string log_msg = std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
            serialize(LogLevel::value::FATAL, file, line, log_msg);
        }
        catch (const std::format_error& e) {
            throw std::runtime_error("Log format error: " + std::string(e.what()));
        }
    };

protected:
    //在这里将日志消息组织起来，并写入文件
    void serialize(LogLevel::value level, const std::string& file, size_t line,
        std::string msgData)
    {
        // std::cout << "Debug:serialize begin\n";
        LogMessage msg(level, file, line, logger_name_, msgData);
        //将msg序列话为一行字符串data
        std::string data = msg.format();


        //if (level == LogLevel::value::FATAL ||
        //    level == LogLevel::value::ERROR)
        //{
        //    try
        //    {
        //        //这里的ret是std::future类型
        //        //第1行：提交任务到线程池，返回 future 对象
        //        auto ret = tp->enqueue(start_backup, data);//将日志信息上传到服务器
        //        //第2行：阻塞等待任务执行完成，获取结果（或确认执行结束）
        //        ret.get();
        //    }
        //    catch (const std::runtime_error& e)
        //    {
        //        // 该线程池没有把stop设置为true的逻辑，所以不做处理
        //        std::cout << __FILE__ << __LINE__ << "thread pool closed" << std::endl;
        //    }
        //}
        // 
        // 
        //获取到string类型的日志信息后就可以输出到异步缓冲区了，异步工作器后续会对其进行刷盘
        Flush(data.c_str(), data.size());

        // std::cout << "Debug:serialize Flush\n";
    }

    void Flush(const char* data, size_t len)
    {
        asyncworker->Push(data, len); // Push函数本身是线程安全的，这里不加锁
    }

    void RealFlush(LogBuffer& buffer)//这个才是真正写入内存的回调函数
    { // 由异步线程进行实际写文件
        if (flushs_.empty())
            return;
        for (auto& e : flushs_)
        {  //e是Flush这个类，即控制把日志输出到哪的类
            //这里才是真正做磁盘永久化，将日志输出到日志文件中，Flush会根据不同的类进行重写
            e->Flush(buffer.Begin(), buffer.ReadableSize());
        }
    }

protected:
    std::mutex mtx_;
    std::string logger_name_;
    std::vector<LogFlush::ptr> flushs_; // 输出到指定方向\
        std::vector<LogFlush> flush_;不能使用logflush作为元素类型，logflush是纯虚类，不能实例化
    AsyncWorker::ptr asyncworker;
};