#include"eventloop.h"
#include"loopthreadpool.h"

#include<functional>
#include<vector>
#include<thread>
#include<memory>
#include<mutex>
#include<condition_variable>


LoopThreadPool::LoopThreadPool(EventLoop *loop) : 
    m_base_loop(loop), m_thread_num(0), m_next_loop_index(0)
{}

LoopThreadPool::~LoopThreadPool()
{}

void LoopThreadPool::start()
{
    //1. 在threadpool中创建线程，且每个线程各自创建一个EventLoop对象，并运行loop函数
    for (int i = 0; i < m_thread_num; ++i)
    {
        m_threads.emplace_back([this](){
            std::unique_ptr<EventLoop> loop(std::make_unique<EventLoop>());          // make_unique() is C++14
            EventLoop *loop_in_this_thread = loop.get();
            {
                std::lock_guard<std::mutex> guard(m_loop_mutex);
                m_loops.emplace_back(std::move(loop));       
                if(m_loops.size() == m_thread_num)  m_loop_cond.notify_one();
            }
            loop_in_this_thread->loop();
        });
    }
    //2. 等待所有线程都创建好EventLoop对象
    {
        std::unique_lock<std::mutex> lock(m_loop_mutex);
        m_loop_cond.wait(lock, [this](){ return m_loops.size() == m_thread_num; });
    }
}

EventLoop* LoopThreadPool::getNextLoop()
{// 必须先调用start()，否则m_loops为空
    std::lock_guard<std::mutex> guard(m_loop_mutex);
    EventLoop *loop = m_base_loop;
    if(m_loops.empty()) return loop; // m_loops为空，说明只有一个线程，即主线程，直接返回主线程的EventLoop对象(loop

    // unique_ptr，不能直接返回，需要用get()获取裸指针
    loop = m_loops[m_next_loop_index++].get(); 
    m_next_loop_index %= m_thread_num;
    return loop;
}
