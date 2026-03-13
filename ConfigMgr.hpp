#pragma once
#include <fstream>  
#include <boost/property_tree/ptree.hpp>  
#include <boost/property_tree/ini_parser.hpp>  
#include <boost/filesystem.hpp>    
#include <map>
#include <iostream>

// --- SectionInfo 结构体定义 (来源于 ConfigMgr.h) ---
struct SectionInfo {
	SectionInfo() {}
	~SectionInfo() {
		_section_datas.clear();
	}

	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}

	SectionInfo& operator = (const SectionInfo& src) {
		if (&src == this) {
			return *this;
		}

		this->_section_datas = src._section_datas;
		return *this;
	}

	std::map<std::string, std::string> _section_datas;

	// 原生运算符重载
	std::string  operator[](const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		return _section_datas.at(key); // 使用 at 确保 const 安全或直接用 []
	}

	// 原生函数名：GetValue
	std::string GetValue(const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		return _section_datas.at(key); //
	}
};

// --- ConfigMgr 类定义 ---
class ConfigMgr
{
public:
	~ConfigMgr() {
		_config_map.clear();
	}

	// 原生运算符重载
	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map.at(section); //
	}

	ConfigMgr& operator=(const ConfigMgr& src) {
		if (&src == this) {
			return *this;
		}

		this->_config_map = src._config_map;
		return *this; // 补全返回语句
	}

	ConfigMgr(const ConfigMgr& src) {
		this->_config_map = src._config_map;
	}

	// 保留原生的单例实现 Inst()
	static ConfigMgr& Inst() {
		static ConfigMgr cfg_mgr;
		return cfg_mgr;
	}

	// 原生函数名：GetValue，实现逻辑来源于 ConfigMgr.cpp
	std::string GetValue(const std::string& section, const std::string& key) {
		if (_config_map.find(section) == _config_map.end()) {
			return "";
		}

		return _config_map.at(section).GetValue(key); //
	}

	// 原生函数名：GetFileOutPath，实现逻辑来源于 ConfigMgr.cpp
	boost::filesystem::path GetFileOutPath() {
		return _static_path;
	}

	std::string GetProjectName() {
		return _config_map["Project"].GetValue("Name");
	}

	std::string GetHostName() {
		return _config_map["Host"].GetValue("Name");
	}

	// 原生函数名：InitPath，实现逻辑来源于 ConfigMgr.cpp
	void InitPath() {
		// 获取当前工作目录
		boost::filesystem::path current_path = boost::filesystem::current_path();
		std::string bindir = _config_map["Output"].GetValue("Path"); //
		std::string staticdir = _config_map["Static"].GetValue("Path"); //
		std::string Project_dir = _config_map["Project"].GetValue("Name"); //
		_static_path = current_path / bindir / staticdir/Project_dir; 
		_bin_path = current_path / bindir; 
		std::map<std::string, std::string> section_config;
		section_config["Path"] = _static_path.string();
		SectionInfo sectionInfo;
		sectionInfo._section_datas = section_config;
		_config_map["Output"] = sectionInfo;
		// 检查路径是否存在并创建
		if (!boost::filesystem::exists(_static_path)) {
			if (boost::filesystem::create_directories(_static_path)) {
				std::cout << "_static_path build successful: " << _static_path.string() << std::endl; //
			}
			else {
				std::cerr << " _static_path build failed:" << _static_path.string() << std::endl; //
			}
		}
		else {
			std::cout << "_static_path build existed: " << _static_path.string() << std::endl; //
		}
	}

private:
	// 私有构造函数，逻辑合并自 ConfigMgr.cpp
	ConfigMgr() {
		// 获取当前工作目录并构建 config.ini 路径
		boost::filesystem::path current_path = boost::filesystem::current_path();
		boost::filesystem::path config_path = current_path / "config.ini"; //
		std::cout << "Config path: " << config_path << std::endl; //

		// 使用 Boost.PropertyTree 读取 INI 文件
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(config_path.string(), pt); //

		// 遍历所有 section
		for (const auto& section_pair : pt) {
			const std::string& section_name = section_pair.first;
			const boost::property_tree::ptree& section_tree = section_pair.second;

			std::map<std::string, std::string> section_config;
			for (const auto& key_value_pair : section_tree) {
				const std::string& key = key_value_pair.first;
				const std::string& value = key_value_pair.second.get_value<std::string>();
				section_config[key] = value;
			}
			SectionInfo sectionInfo;
			sectionInfo._section_datas = section_config;
			_config_map[section_name] = sectionInfo; //
		}

		InitPath();
		// 输出所有的 section 和 key-value 对
		for (const auto& section_entry : _config_map) {
			const std::string& section_name = section_entry.first;
			SectionInfo section_config = section_entry.second;
			std::cout << "[" << section_name << "]" << std::endl;
			for (const auto& key_value_pair : section_config._section_datas) {
				std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl; //
			}
		}

		 //
	}

	// 存储 section 和 key-value 对的 map
	std::map<std::string, SectionInfo> _config_map;
	// static 目录
	boost::filesystem::path _static_path;
	// bin 输出目录
	boost::filesystem::path _bin_path;
};