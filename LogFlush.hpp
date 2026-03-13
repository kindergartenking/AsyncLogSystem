#pragma once
#include <cassert>
#include <fstream>
#include <memory>


#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <Windows.h>

#include <fcntl.h> 
#include "ConfigMgr.hpp"
#include  "Util.hpp"

extern std::string StartTime;

class LogFlush
{
    public:
        using ptr = std::shared_ptr<LogFlush>;
        virtual ~LogFlush() {}
        virtual void Flush(const char* data, size_t len) = 0;//不同的写文件方式Flush的实现不同
};

class StdoutFlush : public LogFlush
{
    public:
        using ptr = std::shared_ptr<StdoutFlush>;
        void Flush(const char* data, size_t len) override {
            std::cout.write(data, len);
        }
};


class FileFlush : public LogFlush
  {
    public:
        using ptr = std::shared_ptr<FileFlush>;
        FileFlush(const std::string& filename) : filename_(filename)
        {
            // 创建所给目录
            std::string output_path_str = ConfigMgr::Inst().GetValue("Output", "Path");
            if (output_path_str.empty()) {
                throw std::runtime_error("配置文件中 Output.Path 为空");
            }
            fs::path output_path(output_path_str);
            
            //full_path_ = output_path / filename_;
            std::string ProjectName = ConfigMgr::Inst().GetProjectName();
            std::string filenameWithProjectAndTime = ProjectName + StartTime + filename_;
            full_path_ = output_path / filenameWithProjectAndTime;
            // 打开文件
            fs_.open(full_path_.string(), std::ios::app | std::ios::binary | std::ios::out | std::ios::ate);
            if (!fs_.is_open()) {
                throw std::runtime_error("打开日志文件失败：" + full_path_.string());
            }

            std::cout << "日志文件打开成功：" << full_path_.string() << std::endl;
        }


        void Flush(const char* data, size_t len) override {
            if (data == nullptr || len == 0 || !fs_.is_open()) {
                std::cerr << __FILE__ << ":" << __LINE__ << " 刷盘参数无效/文件未打开" << std::endl;
                return;
            }

            try {
                // 1. 写入数据（替代 fwrite）
                fs_.write(data, len);

                // 2. 检查写入错误（替代 ferror）
                if (fs_.fail()) {
                    std::cerr << __FILE__ << ":" << __LINE__ << " 写入日志文件失败" << std::endl;
                    return;
                }

                // 3. 强制刷盘（核心：替代 fflush + fsync）
                // 步骤1：刷新 C++ 流缓冲区（替代 fflush）
                fs_.flush();

                // 步骤2：强制刷到物理磁盘（替代 fsync，Windows 兼容）
                if (!FlushFileToDisk()) {
                    std::cerr << __FILE__ << ":" << __LINE__ << " 日志刷盘到磁盘失败" << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << __FILE__ << ":" << __LINE__ << " 刷盘异常：" << e.what() << std::endl;
            }
            
        }

    private:
        // Windows 下强制刷盘到物理磁盘（替代 fsync）
        bool FlushFileToDisk() {
            try {
                // 1. 转换路径为 Windows 宽字符（仅依赖 file_path_，无流操作）
                std::string file_path_str = full_path_.string();
                int wlen = MultiByteToWideChar(CP_UTF8, 0, file_path_str.c_str(), -1, nullptr, 0);
                if (wlen == 0) {
                    std::cerr << "路径转宽字符失败，错误码：" << GetLastError() << std::endl;
                    return false;
                }
                std::wstring wpath(wlen, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, file_path_str.c_str(), -1, &wpath[0], wlen);

                // 2. 共享模式打开文件（仅获取句柄用于刷盘）
                HANDLE hFile = CreateFileW(
                    wpath.c_str(),
                    GENERIC_READ,               // 仅需读权限即可刷盘
                    FILE_SHARE_WRITE | FILE_SHARE_READ, // 共享写/读，不阻塞原有流
                    nullptr,
                    OPEN_EXISTING,              // 只打开已存在的文件（避免创建空文件）
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr
                );

                if (hFile == INVALID_HANDLE_VALUE) {
                    std::cerr << "获取文件句柄失败，错误码：" << GetLastError() << std::endl;
                    return false;
                }

                // 3. 核心：强制内核缓冲区写入物理磁盘（无任何流操作）
                BOOL ret = FlushFileBuffers(hFile);
                CloseHandle(hFile); // 立即释放句柄，避免泄漏

                if (!ret) {
                    std::cerr << "刷盘到磁盘失败，错误码：" << GetLastError() << std::endl;
                    return false;
                }

                return true;
            }
            catch (const std::exception& e) {
                std::cerr << "刷盘异常：" << e.what() << std::endl;
                return false;
            }
        }
        std::string filename_;
        std::ofstream fs_;
        fs::path full_path_;
    };

   

class LogFlushFactory
 {
    public:
        using ptr = std::shared_ptr<LogFlushFactory>;
        template <typename FlushType, typename... Args>
        static std::shared_ptr<LogFlush> CreateLog(Args &&...args)
        {
            // 完美转发参数，创建子类实例，返回基类智能指针，如果参数是右值则触发移动构造，如果是左值则正常开销，这样完美转发可以避免拷贝开销
            return std::make_shared<FlushType>(std::forward<Args>(args)...);
        }
  };
