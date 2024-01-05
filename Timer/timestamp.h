#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <chrono>
#include <ctime>
#include <string>

class Timestamp
{
public:
    // initialize with zero time
    Timestamp() : m_time_point(std::chrono::steady_clock::time_point()) {}
    explicit Timestamp(std::chrono::steady_clock::time_point time_point) : m_time_point(time_point) {}
    ~Timestamp() = default;

    bool operator<(const Timestamp &timestamp) const { return m_time_point < timestamp.m_time_point; }
    bool operator==(const Timestamp &timestamp) const { return m_time_point == timestamp.m_time_point; }

    std::string toString() const
    {
        char buf[32] = {0};
        // auto seconds = std::chrono::duration_cast<std::chrono::seconds>(m_time_point.time_since_epoch());
        // auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(m_time_point.time_since_epoch());
        // auto microseconds_part = microseconds - seconds;
        // auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(seconds));
        // std::tm* tm = std::gmtime(&time);
        // snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06ld",
        //     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        //     tm->tm_hour, tm->tm_min, tm->tm_sec,
        //     microseconds_part.count());
        return buf;
    }

    static Timestamp now() { return Timestamp(std::chrono::steady_clock::now()); }
    static Timestamp addTime(const Timestamp &timestamp, double seconds)
    {
        // 定义一秒的时间段
        using seconds_duration = std::chrono::duration<double>;

        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(seconds_duration(seconds));

        return Timestamp(timestamp.m_time_point + microseconds);
    }

    static int64_t timeDifference(const Timestamp &high, const Timestamp &low)
    {
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(high.m_time_point - low.m_time_point);
        return microseconds.count();
    }
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    std::chrono::steady_clock::time_point m_time_point;
};

#endif // TIMESTAMP_H