#pragma once
#include <vector>
#include <boost/asio.hpp>
#include <queue>

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
class AsioThreadPool 
{
	
public:
    AsioThreadPool(std::size_t size = 3) :stop(false) {
        for (size_t i = 0; i < size; ++i)
        {
            _threads.emplace_back(
                [this]
                {
                    for (;;)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            // 等待任务队列不为空或线程池停止
                            this->condition.wait(lock,
                                [this]
                                { return this->stop || !this->tasks.empty(); });
                            //return 0 阻塞线程
                            if (this->stop && this->tasks.empty())
                                return;
                            //取出任务
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        // 执行任务
                        task();
                    }
                });
        }
    }


    template <class F, class... Args>
    // 该函数用于将一个新任务添加到任务队列中，并返回一个 std::future 对象，用于获取任务的执行结果。
    // 使用 std::packaged_task 将传入的函数 f 和参数 args 打包成一个可调用对象。
    // 通过 task->get_future() 获取一个 std::future 对象，用于异步获取任务的返回值。
    // 使用 std::unique_lock<std::mutex> 加锁，确保线程安全地访问任务队列。
    // 检查线程池是否已经停止，如果停止则抛出异常。
    // 将打包好的任务封装成一个 lambda 函数，并添加到任务队列中。
    // 调用 condition.notify_one() 唤醒一个等待的线程，通知它有新任务可用。
    // 返回 std::future 对象。
    auto enqueue(F&& f, Args &&...args)
        -> std::future<typename std::invoke_result_t<F(Args...)>>
    {
        using return_type = typename std::invoke_result_t<F(Args...)>;

        // 创建一个打包任务
        //3. 封装任务：把带参任务转为无参的packaged_task（核心）
        //std::packaged_task 是 C++11 提供的 “任务包装器”—— 它能把一个可调用对象（函数 /lambda）包装成 “可异步执行的任务”
        //并绑定一个 std::future，任务执行的结果会自动存入这个 future。
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        // 4. 获取future：通过packaged_task拿到任务返回值的“未来对象”：给异步函数加 “结果接收器”+“状态监控器”
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop)// 如果线程池已停止，抛出异常
                throw std::runtime_error("enqueue on stopped ThreadPool");
            // 将任务添加到任务队列
            tasks.emplace([task]()  //task是共享指针，lambda捕获后仍能访问
                { (*task)(); });//执行无参任务，不是立即执行，而是加入队列而已，函数对象而不是函数调用
        }
        condition.notify_one();
        return res;
    }

    ~AsioThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& thread : _threads)
        {
            thread.join();
        }
    }

private:
	
	
	
	std::vector<std::thread> _threads;
	std::queue<std::function<void()>> tasks; // 任务队列
	std::mutex queue_mutex;                  // 任务队列的互斥锁
	std::condition_variable condition;       // 条件变量，用于任务队列的同步
    bool stop;
};

