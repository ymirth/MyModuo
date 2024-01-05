#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <chrono>
#include <ctime>

#include "timestamp.h"

class Timer
{
public:
    typedef std::function<void()> TimerCallback;
    Timer(const TimerCallback &&cb, Timestamp when, double interval)
        : m_callback(std::move(cb)), m_expiration(when), m_interval(interval), m_repeat(interval > 0.0) {}
    ~Timer() = default;

#pragma region delete copy and move
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;
    Timer(Timer &&) = delete;
    Timer &operator=(Timer &&) = delete;
#pragma endregion

    void run() const { m_callback(); }

    Timestamp expiration() const { return m_expiration; }
    bool repeat() const { return m_repeat; }
    void restart(Timestamp now)
    {
        m_expiration = Timestamp::addTime(now, m_interval);
    }

    // for priority_queue
    bool operator<(const Timer &timer) const { return m_expiration < timer.m_expiration; }
private:
    const TimerCallback m_callback;
    Timestamp m_expiration;    // when to run
    const double m_interval;   // interval between repeat
    const bool m_repeat;
};

#endif // TIMER_H