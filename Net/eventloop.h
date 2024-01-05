#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <sys/epoll.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <iostream>

#include"epoller.h"

class RBTreeTimer;
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

    bool isInLoopThread() const
    {
        return m_thread_id == std::this_thread::get_id();
    }
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            std::cout<<"EventLoop was created in thread {" <<m_thread_id<<"}, the current thread id is {"<<std::this_thread::get_id()<<"}"<<std::endl;
            abort();
        }
    }
    
    void doPendingFunctors();
};

#endif