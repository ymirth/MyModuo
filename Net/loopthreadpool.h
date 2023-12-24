#ifndef LOOP_THREAD_POOL_H
#define LOOP_THREAD_POOL_H

#include<vector>
#include<thread>
#include<memory>
#include<mutex>
#include<condition_variable>

class EventLoop;

class LoopThreadPool
{
public:
    explicit LoopThreadPool(EventLoop *loop);
    ~LoopThreadPool();

    #pragma region delete copy and move
    LoopThreadPool(const LoopThreadPool&) = delete;
    LoopThreadPool& operator=(const LoopThreadPool&) = delete;
    LoopThreadPool(LoopThreadPool&&) = delete;
    LoopThreadPool& operator=(LoopThreadPool&&) = delete;
    #pragma endregion delete copy and move

    void start();
    void setThreadNum(int num_threads) { m_thread_num = num_threads; }
    EventLoop* getNextLoop();
private:
    EventLoop *m_base_loop;
    int m_thread_num;
    int m_next_loop_index;
    std::vector<std::unique_ptr<EventLoop>> m_loops;
    std::vector<std::thread> m_threads;
    std::mutex m_loop_mutex;
    std::condition_variable m_loop_cond; // wait for all threads to start; used in start()
};




#endif