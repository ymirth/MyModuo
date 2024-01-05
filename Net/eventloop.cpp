#include<sys/eventfd.h>
#include<unistd.h>

#include<functional>
#include<thread>
#include<memory>
#include<mutex>
#include<string>

#include"channel.h"
#include"epoller.h"
#include"rbtreetimer.h"

#include"eventloop.h"

EventLoop::EventLoop()
    :m_looping(false),
    m_thread_id(std::this_thread::get_id()),
    m_epoller(new Epoller()),
    m_wakeup_fd(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),// count = 0: no need to block
    m_wakeup_channel(new Channel(this, m_wakeup_fd)),
    m_timer(new RBTreeTimer(this)),
    m_calling_pending_functors(false),
    m_active_channels(0), //create a vector with 0 elements
    m_pending_functors(0) //create a vector with 0 elements
{
    assert(m_wakeup_fd > 0);
    m_wakeup_channel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    m_wakeup_channel->enableReading(); // 1.event = EPOLLIN | EPOLLET 2.update: epoll_ctl
}

EventLoop::~EventLoop()
{
    if(m_looping) m_looping = false;
    // m_wakeup_channel->disableAll();
    // m_epoller->removeChannel(m_wakeup_channel.get());
    ::close(m_wakeup_fd);
}

void EventLoop::quit()
{
    m_looping = false;
    if(!isInLoopThread()) wakeup();
}

void EventLoop::loop()
{
    assert(isInLoopThread());
    m_looping = true;
    while(m_looping)
    {
        m_active_channels.clear();
        m_epoller->poll(m_active_channels);
        for(const auto& channel : m_active_channels)
        {
            channel->handleEvent();
        }
        this->doPendingFunctors();
    }
}

void EventLoop::runInLoop(std::function<void()> cb)
{
    if(isInLoopThread()) cb();
    else queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(std::function<void()> cb)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending_functors.push_back(std::move(cb));
    }
    if(!isInLoopThread() || m_calling_pending_functors) wakeup();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(m_wakeup_fd, &one, sizeof(one));
    if(n != sizeof(one))
    {
        //perror("EventLoop::wakeup() writes " + std::to_string(n) + " bytes instead of 8");
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(m_wakeup_fd, &one, sizeof(one));
    if(n != sizeof(one))
    {
        // perror("EventLoop::handleRead() reads " + std::to_string(n) + " bytes instead of 8");
    }
}


void EventLoop::doPendingFunctors()
{
    std::vector<std::function<void()>> functors;
    m_calling_pending_functors = true;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        functors.swap(m_pending_functors);
    }
    for(const auto& functor : functors)
    {
        functor();
    }
    m_calling_pending_functors = false;
}