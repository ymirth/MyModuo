#ifndef LOGGING_H
#define LOGGING_H
#include <cstring>
#include <string>
#include <thread>

#include "timestamp.h"
#include "logstream.h"

class SourceFile
{
public:
    SourceFile(const char *filename)
        : m_data(filename)
    {
        const char *slash = strrchr(filename, '/');
        if (slash)
        {
            m_data = slash + 1;
        }
        m_size = static_cast<int>(strlen(m_data));
    }

    const char *m_data;
    int m_size;
};

class Logger
{
public:
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARN,
        ERROR,
    };

    Logger(SourceFile file, int line, LogLevel level)
        : m_impl(file, line, level) {}

    LogStream &stream() { return m_impl.stream(); }

    typedef void (*OutputFunc)(const char *msg, int len);
    typedef void (*FlushFunc)();

private:
    class Impl
    {
    public:
        using LogLevel = Logger::LogLevel;
        Impl(const SourceFile &source, int line, LogLevel level);
        ~Impl();
        void formatTime();
        const char *getLogLevel();
        void finish() { m_stream << " - " << m_basename.m_data << ':' << m_line << '\n'; }
        LogStream &stream() { return m_stream; }

    private:
        LogStream m_stream;
        LogLevel m_level;
        int m_line;
        std::thread::id m_tid;
        int64_t m_lastSecond;
        SourceFile m_basename;
    };
    Impl m_impl;
};

Logger::LogLevel logLevel();
void setLogLevel(Logger::LogLevel level);
const char *ErrorToString(int error);

#define LOG_DEBUG                              \
    if (logLevel() <= Logger::LogLevel::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::DEBUG).stream()

#define LOG_INFO                              \
    if (logLevel() <= Logger::LogLevel::INFO) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::INFO).stream()

#define LOG_WARN Logger(__FILE__, __LINE__, Logger::LogLevel::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::LogLevel::ERROR).stream()

#endif