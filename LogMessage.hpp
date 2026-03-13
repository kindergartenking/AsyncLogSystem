#pragma once

#include <memory>
#include <thread>
#include <ctime>

#include <iomanip>   
#include <format> 
#include <sstream>
#include "ConfigMgr.hpp"
#include "Level.hpp"
#include "Util.hpp"

extern std::string StartTime;


    struct LogMessage
    {
        using ptr = std::shared_ptr<LogMessage>;
        LogMessage() = default;
        LogMessage(LogLevel::value level, std::string file, size_t line,
            std::string name, std::string payload)
            : name_(name),
            file_name_(file),
            payload_(payload),
            level_(level),
            line_(line),
            ctime_(Date::Now()),
            tid_(std::this_thread::get_id()),
            ProjectName(ConfigMgr::Inst().GetProjectName()),
            HostName(ConfigMgr::Inst().GetHostName()),
            starttime(StartTime)
        {
        }


        //format() 函数的最终目标是生成一条结构化、易读的日志字符串，典型输出示例：
        //[15:30:45][140708234567890][DEBUG][upload_module][upload.cpp:123]	用户上传文件：test.log，大小：1024 字节
        std::string format()
        {
            std::tm t{};
            localtime_s(&t, &ctime_); // 保留你原有的线程安全时间函数

            // 1. 格式化时间（C++11 标准，无风险）
            std::ostringstream time_ss;
            time_ss << std::put_time(&t, "%H:%M:%S");
            HostName = "HostName:" + HostName;
            starttime = "StartTime:" + StartTime;
            ProjectName = "ProjectName:" + ProjectName;
            // 2. 流式拼接日志（替代 std::format）
            std::ostringstream ret;
            ret << "[" <<HostName<< "][" <<starttime<< "][" <<ProjectName << "][" << time_ss.str() << "]["
                << tid_ << "]["
                << LogLevel::ToString(level_) << "]["
                << name_ << "]["
                << file_name_ << ":" << line_ << "]\t"
                << payload_ << "\n";

            return ret.str();
        }

        size_t line_;           // 行号
        time_t ctime_;          // 时间
        std::string file_name_; // 文件名
        std::string name_;      // 日志器名
        std::string payload_;   // 日志内容
        std::thread::id tid_;   // 线程id
        LogLevel::value level_; // 等级
        std::string ProjectName;//项目名称
        std::string HostName;//主机名称
        std::string starttime;//主机名称
    };
