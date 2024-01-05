#include <sys/timerfd.h>
#include <cstring>

#include <functional>
#include <chrono>
#include <queue>

#include "channel.h"
#include "eventloop.h"
#include "rbtreetimer.h"

RBTreeTimer::RBTreeTimer(EventLoop *loop) : m_loop(loop),
                                            m_timer_fd(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
                                            m_timer_channel(new Channel(loop, m_timer_fd))
{
    m_timer_channel->setReadCallback(std::bind(&RBTreeTimer::handleRead, this));
    m_timer_channel->enableReading();
}

RBTreeTimer::~RBTreeTimer()
{
    // m_timer_channel->disableAll();
    // m_loop->remove(m_timer_channel.get());
    ::close(m_timer_fd);
}

void RBTreeTimer::addTimer(const TimerCallback &&cb, const Timestamp &when, double interval)
{
    std::shared_ptr<Timer> timer = std::make_shared<Timer>(std::move(cb), when, interval);
    m_loop->runInLoop(std::bind(&RBTreeTimer::addTimerInLoop, this, timer));
}

void RBTreeTimer::addTimerInLoop(const std::shared_ptr<Timer> &timer)
{
    m_loop->assertInLoopThread();
    bool earliestChanged = insertTimer(timer);
    if (earliestChanged)
    {
        resetTimerFd(timer);
    }
}

bool RBTreeTimer::insertTimer(const std::shared_ptr<Timer> &timer)
{
    Timestamp when = timer->expiration();
    auto minTimer = m_timer.begin();
    m_timer.insert(timer);
    return m_timer.empty() || when < (*minTimer)->expiration();
}

void RBTreeTimer::resetTimerFd(const std::shared_ptr<Timer> &timer)
{
    m_loop->assertInLoopThread();
    struct itimerspec new_value;
    struct itimerspec old_value;
    memset(&new_value, 0, sizeof(new_value));
    memset(&old_value, 0, sizeof(old_value));

    Timestamp now(Timestamp::now());
    Timestamp expiration = timer->expiration();

    // diff
    int64_t diff = Timestamp::timeDifference(expiration, now);
    new_value.it_value.tv_sec = static_cast<time_t>(diff / Timestamp::kMicroSecondsPerSecond);
    new_value.it_value.tv_nsec = static_cast<long>(diff % Timestamp::kMicroSecondsPerSecond * 1000);
    
    int ret = ::timerfd_settime(m_timer_fd, 0, &new_value, &old_value);
    if (ret)
    {
        std::cout << "timerfd_settime() error: " << strerror(errno) << std::endl;
    }
}

void RBTreeTimer::handleRepeatTimer(const std::shared_ptr<Timer> &timer)
{
    if (timer->repeat())
    {
        timer->restart(Timestamp::now());
        insertTimer(timer);
    }
}

void RBTreeTimer::handleRead()
{
    m_loop->assertInLoopThread();
    auto readfd = [this]()
    {
        uint64_t read_bytes;
        auto readn = ::read(m_timer_fd, &read_bytes, sizeof(read_bytes));
        if (readn != sizeof(read_bytes))
        {
            std::cout << "Timer::handleRead() reads " << readn << " bytes instead of 8" << std::endl;
        }
    };
    readfd();

    Timestamp now(Timestamp::now());
    auto end = m_timer.lower_bound(std::make_shared<Timer>(nullptr, now, 0.0));
    std::vector<std::shared_ptr<Timer>> expired = std::vector<std::shared_ptr<Timer>>(m_timer.begin(), end);
    m_timer.erase(m_timer.begin(), end);
    for (const auto &timer : expired)
    {
        timer->run();
        handleRepeatTimer(timer);
    }
    if(!m_timer.empty())
    {
        resetTimerFd(*m_timer.begin());
    }
}





