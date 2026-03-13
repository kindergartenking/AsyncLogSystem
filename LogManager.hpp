#include<unordered_map>
#include"AsyncLogger.hpp"




class LoggerManager
{
public:
    static LoggerManager& GetInstance()
    {
        static LoggerManager eton;
        return eton;
    }

    bool LoggerExist(const std::string& name)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = loggers_.find(name);
        if (it == loggers_.end())
            return false;
        return true;
    }

    void AddLogger(const AsyncLogger::ptr&& AsyncLogger)
    {
        if (LoggerExist(AsyncLogger->Name()))
            return;
        std::unique_lock<std::mutex> lock(mtx_);
        loggers_.insert(std::make_pair(AsyncLogger->Name(), AsyncLogger));
    }

    AsyncLogger::ptr GetLogger(const std::string& name)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = loggers_.find(name);
        if (it == loggers_.end())
            return AsyncLogger::ptr();
        return it->second;
    }

   

private:
    LoggerManager()
    {
        
    }

private:
    std::mutex mtx_;
    std::unordered_map<std::string, AsyncLogger::ptr> loggers_; // ¥Ê∑≈»’÷æ∆˜
};