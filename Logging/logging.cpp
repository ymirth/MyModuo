#include <ctime>
#include <chrono>

#include "timestamp.h"
#include "logging.h"


const char *ErrorToString(int err)
{
    return strerror(err);
}

static const int kLogLevelStringLen = 6;

static void DefaultOutput(const char *msg, int len)
{
    fwrite(msg, 1, len, stdout);
}

static void DefaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = DefaultOutput;
Logger::FlushFunc g_flush = DefaultFlush;
Logger::LogLevel g_logLevel = Logger::LogLevel::INFO;

void setOutput(Logger::OutputFunc out) { g_output = out; }
void setFlush(Logger::FlushFunc flush) { g_flush = flush; }

void setLogLevel(Logger::LogLevel level) { g_logLevel = level; }
Logger::LogLevel logLevel() { return g_logLevel; }

Logger::Impl::Impl(const SourceFile& file, int line, LogLevel level)
    : m_stream(),
      m_level(level),
      m_line(line),
      m_basename(file),
      m_tid(std::this_thread::get_id())
{
    auto now = std::chrono::system_clock::now();
    m_lastSecond = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    formatTime();
}

Logger::Impl::~Impl()
{
    finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.len());
    if (m_level == Logger::LogLevel::ERROR)
    {
        g_flush();
    }
}

const char* Logger::Impl::getLogLevel()
{
    switch (m_level)
    {
    case Logger::LogLevel::DEBUG:
        return "DEBUG";
    case Logger::LogLevel::INFO:
        return "INFO";
    case Logger::LogLevel::WARN:
        return "WARN";
    case Logger::LogLevel::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void Logger::Impl::formatTime()
{
    Timestamp now = Timestamp::now();
    int64_t microsecondsSinceEpoch = now.microsecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microsecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microsecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
    if (seconds != m_lastSecond)
    {
        m_lastSecond = seconds;
    }
    auto time = now.toDefaultLogString();
    m_stream << time << "." << microseconds << " ";
    //m_tid to string
    std::string tid = std::to_string(std::hash<std::thread::id>{}(m_tid));
    m_stream << tid << " ";
    m_stream << getLogLevel() << " ";
}

// m_lastSecond: 上一次的秒数