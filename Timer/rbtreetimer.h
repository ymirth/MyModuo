#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <functional>
#include <memory>
#include <chrono>
#include <ctime>
#include <set>

#include "timestamp.h"
#include "timer.h"

class Channel;
class EventLoop;

// noncopyable
class RBTreeTimer
{
public:
    typedef std::function<void()> TimerCallback;
    explicit RBTreeTimer(EventLoop *loop);
    ~RBTreeTimer();
#pragma region delete copy and move
    RBTreeTimer(const RBTreeTimer &) = delete;
    RBTreeTimer &operator=(const RBTreeTimer &) = delete;
    RBTreeTimer(RBTreeTimer &&) = delete;
    RBTreeTimer &operator=(RBTreeTimer &&) = delete;
#pragma endregion

public:
    // handleRead() -> handleRepeatTimer() -> resetTimerfd(Timestamp expiration)
    void handleRead();                                         // handle read event in eventloop

    // Thread safe API for adding timer
    void addTimer(const TimerCallback &&cb, const Timestamp &when, double interval);
    void addTimerInLoop(const std::shared_ptr<Timer> &timer);  // must be called in loop thread
private:
    bool insertTimer(const std::shared_ptr<Timer> &timer);
    void resetTimerFd(const std::shared_ptr<Timer> &timer);
    void handleRepeatTimer(const std::shared_ptr<Timer> &timer);
    void enableTimerfd();                                      // enable timerfd

    // void resetTimerfd(Timestamp expiration);

private:
    struct cmp
    {
        bool operator()(const std::shared_ptr<Timer> &lhs, const std::shared_ptr<Timer> &rhs) const
        {
            return std::less<Timer>()(*lhs, *rhs);
        }
    };

    EventLoop *m_loop;
    std::unique_ptr<Channel> m_timer_channel;
    std::set<std::shared_ptr<Timer>, cmp> m_timer; // the first element is the earliest timer
    int m_timer_fd;
};

#endif // HEAP_TIMER_H