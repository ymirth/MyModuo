#ifndef ASYNCLOGGING_H
#define ASYNCLOGGING_H

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "logstream.h"
#include "logging.h"
#include "logfile.h"

static const double kFlushInterval = 3.0;
static const int64_t kRollSize = 1024 * 1024 * 32; // 32MB

class AsyncLogging
{
public:
    using Buffer = FixedBuffer<kLargeBuffer>;
    using LogFilePtr = std::unique_ptr<LogFile>;
    using BufferPtr = std::unique_ptr<Buffer>;
    using BufferVector = std::vector<BufferPtr>;

    AsyncLogging(const std::string &filepath, off_t rollSize = kRollSize, int flushInterval = 3)
        : m_running(false),
          m_filepath(filepath),
          m_currentFile(new LogFile(filepath, rollSize, flushInterval)),
          m_currentBuffer(new Buffer),
          m_nextBuffer(new Buffer){}
    ~AsyncLogging()
    {
        if (m_running)
        {
            stop();
        }
    }
    void stop()
    {
        if(m_running)
        {
            m_running = false;
            m_cond.notify_one();
            m_thread.join();
        }
    }
    void startAsyncLogging()
    {
        m_running = true;
        m_thread = std::thread(std::bind(&AsyncLogging::threadFunc, this));
    }
    void append(const char* data, int len);
    void flush();
    void threadFunc();
private:
    bool m_running;
    const std::string m_filepath;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::thread m_thread;
    LogFilePtr m_currentFile;
    BufferPtr m_currentBuffer;
    BufferPtr m_nextBuffer;
    BufferVector m_buffers;

};

#endif