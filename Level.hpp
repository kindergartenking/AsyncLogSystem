#pragma once
#include <string>



class LogLevel {
public:
    enum class value { DEBUG, INFO,Error, WARN, FATAL };

    // 提供日志等级的字符串转换接口
    static const char* ToString(value level) {
        switch (level) {
        case value::DEBUG:
            return "DEBUG";
        case value::INFO:
            return "INFO";
        case value::WARN:
            return "WARN";
        case value::Error:
            return "ERROR";
        case value::FATAL:
            return "FATAL";
        default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }
};