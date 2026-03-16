#pragma once
#include <memory>
#include <mutex>
#include <iostream>

//template <typename T>
//class Singleton {
//protected:
//	Singleton() = default;
//	Singleton(const Singleton<T>&) = delete;
//	Singleton& operator=(const Singleton<T>& st) = delete;
//	
//	static std::shared_ptr<T> _instance;
//public:
//	static std::shared_ptr<T> GetInstance() {
//		static std::once_flag s_flag;
//		std::call_once(s_flag, [&]() {
//			_instance = std::make_shared<T>(new T);
//			});
//
//		return _instance;
//	}
//	void PrintAddress() {
//		std::cout << _instance.get() << endl;
//	}
//	virtual ~Singleton() = default;
//};
//
//template <typename T>
//std::shared_ptr<T> Singleton<T>::_instance = nullptr;


template <typename T>
class Singleton {
public:
    // 禁用拷贝和赋值，禁止外部实例化
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    // 【和你之前的ConfigMgr完全兼容】统一用Inst()获取实例
    // C++11保证静态局部变量的初始化是线程安全的
    static T& GetInstance() {
        static T instance; // 静态局部对象，程序结束自动析构
        return instance;
    }

protected:
    // 保护构造/析构：子类可访问，外部无法实例化，完全避开权限冲突
    Singleton() = default;
    virtual ~Singleton() = default;
};