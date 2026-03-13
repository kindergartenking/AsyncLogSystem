#include <iostream>
#include <string>



#include <Windows.h>
#include "LogFlush.hpp"
#include "LogManager.hpp"
#include "ConfigMgr.hpp"
#include "Singleton.h"

std::string get_time_str_YYYYMMDDHHMMSS() {
    // 1. 获取当前系统时间戳
    time_t now = time(nullptr);
    tm local_tm{};  // 初始化空结构体，避免脏数据

    // 2. Windows专属线程安全时间转换（localtime_s）
    errno_t err = localtime_s(&local_tm, &now);
    if (err != 0) {
        return "00000000000000";  // 转换失败返回全0，便于排查
    }

    // 3. 格式化到字符数组（高效，无内存冗余）
    char time_buf[15] = { 0 };  // 14位字符 + 1位结束符
    snprintf(
        time_buf, sizeof(time_buf),
        "%04d%02d%02d%02d%02d%02d",
        local_tm.tm_year + 1900,  // 年份：tm_year是1900年起的偏移量
        local_tm.tm_mon + 1,      // 月份：0-11 → 1-12
        local_tm.tm_mday,         // 日期：1-31
        local_tm.tm_hour,         // 小时：0-23
        local_tm.tm_min,          // 分钟：0-59
        local_tm.tm_sec           // 秒：0-59
    );

    return std::string(time_buf);
}

std::string StartTime = get_time_str_YYYYMMDDHHMMSS();


int main() {
    std::cout << "=== ConfigMgr  ===" << std::endl;
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    auto& cfg = ConfigMgr::Inst();
   // auto fileFlush = LogFlushFactory::CreateLog<FileFlush>("test.log");
    //fileFlush->Flush("出现异常",8);
    //auto CMDFlush = LogFlushFactory::CreateLog<StdoutFlush>();
    //std::cout << StartTime;
    //CMDFlush->Flush("11111",5);

    auto fileFlush = LogFlushFactory::CreateLog<FileFlush>("test.log");
    vector<std::shared_ptr<LogFlush>> flush_;
    flush_.emplace_back(fileFlush);
    //如果要两个日志器生成不同的log文件，就要重新设置fileFlush
    auto fileFlushAnother = LogFlushFactory::CreateLog<FileFlush>("testanother.log");
    vector<std::shared_ptr<LogFlush>> flushanother;
    flushanother.emplace_back(fileFlushAnother);
    LoggerManager::GetInstance().AddLogger(std::make_shared<AsyncLogger>("TestLogger",flush_));
    LoggerManager::GetInstance().GetLogger("TestLogger")->Debug("main.cpp",53,"测试日志能否正确写入");
    LoggerManager::GetInstance().AddLogger(std::make_shared<AsyncLogger>("AnotherTestLogger", flushanother));
    LoggerManager::GetInstance().GetLogger("AnotherTestLogger")->Debug("main.cpp", 59, "测试日志能否正确写入另一个日志");
    return 0;
}