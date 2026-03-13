#pragma once
#include <ctime>  
     // 标准 C++ 时间头文件（替代 <time.h>，更符合 C++ 规范）

 // Windows 系统 API 头文件（用于修正时区/精度问题）
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
#include <fstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <Windows.h>  
namespace fs = boost::filesystem;
namespace sys = boost::system;
class Date
{
public:
    
    static time_t Now()
    {   
        return std::time(nullptr);
    }
};


class File
{
public:
    //这里传入的要完整路径
    static bool Exists(const std::string& filename)
    {
        return fs::exists(fs::path(filename));
    }

    //提取路径的目录部分
    static std::string Path(const std::string& filename)
    {
        if (filename.empty())
            return "";
        int pos = filename.find_last_of("/\\");
        if (pos != std::string::npos)
            return filename.substr(0, pos + 1);
        return "";
    }

    //创建多级目录
    static void CreateDirectory(const std::string& pathname)
    {
        // 1. 空路径校验
        if (pathname.empty()) {
            std::cerr << "错误：文件所给路径为空！" << std::endl;
            return;
        }

        // 2. 转换为 Boost 路径对象（自动兼容 / 和 \ 分隔符）
        fs::path dir_path(pathname);

        // 3. 路径已存在则直接返回（无需创建）
        if (fs::exists(dir_path)) {
            return;
        }

        // 4. 核心：Boost 自带递归创建目录（一行替代原手动循环逻辑）
        //    第二个参数：错误码（避免抛异常，更友好）
        sys::error_code ec;
        bool create_ok = fs::create_directories(dir_path, ec);

        // 5. 创建失败时输出错误信息
        if (!create_ok) {
            std::cerr << "创建目录失败：" << pathname
                << "，错误原因：" << ec.message() << std::endl;
        }
    }

    int64_t FileSize(const std::string& filename)
    {
        // 转换为 Boost 路径对象（兼容 / 和 \ 分隔符）
        fs::path file_path(filename);

        // 错误码（避免抛异常，更友好）
        sys::error_code ec;

        // 核心：Boost 获取文件大小（字节数）
        uintmax_t file_size = fs::file_size(file_path, ec);

        // 错误处理
        if (ec) {
            std::cerr << "获取文件大小失败：" << filename
                << "，错误原因：" << ec.message() << std::endl;
            return -1;
        }

        // 转换为 int64_t 返回（兼容原接口）
        return static_cast<int64_t>(file_size);
    }

    // ========== 2. 获取文件内容（兼容 Boost 且优化逻辑） ==========
    bool GetContent(std::string* content, const std::string& filename)
    {
        // 前置校验：content 指针不能为空
        if (content == nullptr) {
            std::cerr << "错误：content 指针为空！" << std::endl;
            return false;
        }

        // 校验文件是否存在（Boost 实现）
        if (!fs::exists(fs::path(filename))) {
            std::cerr << "文件不存在：" << filename << std::endl;
            return false;
        }

        // 打开文件（二进制模式，兼容所有文件类型）
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs.is_open()) {
            std::cerr << "文件打开失败：" << filename << std::endl;
            return false;
        }

        // 读入文件内容（优化逻辑：无需手动 seekg + 依赖 FileSize）
        try {
            // 方式1：直接读取全部内容（无需先获取文件大小，更鲁棒）
            content->assign(std::istreambuf_iterator<char>(ifs),
                std::istreambuf_iterator<char>());
        }
        catch (const std::exception& e) {
            std::cerr << __FILE__ << ":" << __LINE__ << " - 读取文件内容错误："
                << e.what() << std::endl;
            ifs.close();
            return false;
        }

        ifs.close();
        return true;
    }
}; // class file


class JsonUtil
{
public:
    // ========== 1. JSON 对象 → 字符串（序列化） ==========
    // val: 待序列化的 Json::Value 对象
    // str: 输出参数，存储序列化后的 JSON 字符串
    // 返回值：true 成功 / false 失败
    static bool Serialize(const Json::Value& val, std::string* str)
    {
        if (str == nullptr) {
            std::cerr << "错误：Serialize 输出字符串指针为空！" << std::endl;
            return false;
        }

        try {
            // ========== 2. 旧版 JsonCpp 核心序列化逻辑 ==========
            // 方案1：紧凑格式（无缩进，适合网络传输/文件存储）
            Json::FastWriter writer;
            // 方案2：格式化格式（带缩进，适合调试/可读性）
            // Json::StyledWriter writer;

            // 执行序列化（核心调用）
            *str = writer.write(val);

            // ========== 3. 空值校验（避免序列化空对象） ==========
            if (str->empty()) {
                std::cerr << "警告：序列化结果为空字符串（JSON 对象可能为空）！" << std::endl;
                // 空对象序列化也算成功，返回 true；若需严格校验，可改为 return false
            }

            return true;
        }
        catch (const std::exception& e) {
            // ========== 4. 异常捕获（Windows 下避免程序崩溃） ==========
            std::cerr << "序列化异常：" << e.what() << std::endl;
            return false;
        }
    }

    // ========== 2. 字符串 → JSON 对象（反序列化） ==========
    // str: 待解析的 JSON 字符串
    // val: 输出参数，存储解析后的 Json::Value 对象
    // 返回值：true 成功 / false 失败
    static bool UnSerialize(const std::string& str, Json::Value* val)
    {
        if (val == nullptr) {
            std::cerr << "错误：UnSerialize 输出 Json::Value 指针为空！" << std::endl;
            return false;
        }
        // 校验输入字符串不为空
        if (str.empty()) {
            std::cerr << "错误：待解析的 JSON 字符串为空！" << std::endl;
            return false;
        }

        try {
            // ========== 2. 旧版 JsonCpp 核心反序列化逻辑 ==========
            Json::Reader reader; // 旧版核心解析器
            // 执行解析：将字符串解析到 val 指向的 Json::Value 对象
            bool parse_ok = reader.parse(str, *val);

            // ========== 3. 解析失败处理（输出详细错误信息） ==========
            if (!parse_ok) {
                
                return false;
            }

            return true;
        }
        catch (const std::exception& e) {
            // ========== 4. 异常捕获（Windows 下避免程序崩溃） ==========
            std::cerr << "反序列化异常：" << e.what() << std::endl;
            return false;
        }
    }
        
    
};