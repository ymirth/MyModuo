#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <sys/epoll.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <iostream>
#include <chrono>


#include"epoller.h"
#include"timestamp.h"
#include"rbtreetimer.h"
#include"logging.h"

class Channel;

class EventLoop
{
private:
    bool m_looping;
    std::thread::id m_thread_id;
    std::mutex m_mutex;
    std::unique_ptr<Epoller> m_epoller;
    int m_wakeup_fd;
    std::unique_ptr<Channel> m_wakeup_channel; // bind to m_wakeup_fd

    std::unique_ptr<RBTreeTimer> m_timer;

    bool m_calling_pending_functors; // indicate: is calling pending functors
    std::vector<Channel *> m_active_channels;
    std::vector<std::function<void()>> m_pending_functors;

public:
    EventLoop();
    ~EventLoop();

#pragma region delete copy and move
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    EventLoop(EventLoop &&) = delete;
    EventLoop &operator=(EventLoop &&) = delete;
#pragma endregion delete copy and move

    void loop();
    void quit();

    void runInLoop(std::function<void()> cb);
    void queueInLoop(std::function<void()> cb);
    void wakeup();
    void handleRead();

    void update(Channel *channel) { m_epoller->updateChannel(channel); }
    void remove(Channel *channel) { m_epoller->removeChannel(channel); }
    bool hasChannel(Channel *channel) const{    return m_epoller->hasChannel(channel);}

    // Timer
    void runAfter(const double second, std::function<void()> cb)
    {
        auto now = Timestamp::now();
        Timestamp when = Timestamp::addTime(now, second);
        m_timer->addTimer(std::move(cb), when, 0.0);
    }
    void runEvery(const double second, std::function<void()> cb)
    {
        Timestamp when = Timestamp::addTime(Timestamp::now(), second);
        m_timer->addTimer(std::move(cb), when, second);
    }
    


    bool isInLoopThread() const
    {
        return m_thread_id == std::this_thread::get_id();
    }
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            // std::cout<<"EventLoop was created in thread {" <<m_thread_id<<"}, the current thread id is {"<<std::this_thread::get_id()<<"}"<<std::endl;
            LOG_ERROR<<"EventLoop was created in thread {" << std::hash<std::thread::id>{}(m_thread_id)<<"}, the current thread id is {"<<std::hash<std::thread::id>{}(std::this_thread::get_id())<<"}\n";
            abort();
        }
    }
    
    void doPendingFunctors();
};

#endif