#include <thread>

#include "logfile.h"
static std::string getLogFileName(const std::string &basename,const Timestamp &now)
{
    std::string filename = "./log/";
    filename += basename + " ";
    filename += now.toDefaultLogString() + " ";
    //filename增加线程信息
    auto pid = std::this_thread::get_id();
    filename += std::to_string(std::hash<std::thread::id>{}(pid)); // Method2: oss << pid; oss.str();
    filename += ".log";
    return filename;
}


LogFile::LogFile(const std::string &basename, off_t rollSize, int flushInterval, bool threadSafe)
    : m_fp(::fopen(basename.c_str(), "ae")),
      m_writtenBytes(0),
      m_rollSize(rollSize),
      m_flushInterval(flushInterval * Timestamp::kMicroSecondsPerSecond)
{
    if (!m_fp)
    {
        std::string defaultPath = getLogFileName(m_basename, Timestamp::now());
        m_fp = ::fopen(defaultPath.c_str(), "ae");
    }
}

LogFile::~LogFile()
{
    flush();
    ::fclose(m_fp);
}

void LogFile::append(const char *data, int len)
{
    int pos{};
    while (pos != len)
    {
        pos += static_cast<int>(::fwrite_unlocked(data + pos,
                                                  sizeof(char), len - pos, m_fp));
    }
    Timestamp now = Timestamp::now();
    if(len != 0)
    {
        m_writtenBytes += len;
    }
    if(Timestamp::timeDifference(now, m_lastFlush) > m_flushInterval)
    {
        flush();
        m_lastFlush = now;
    }
    else
    {
        if(m_writtenBytes > m_rollSize)
        {
            rollFile();
        }
    }
}

void LogFile::rollFile()
{
    ::fclose(m_fp);
    Timestamp now = Timestamp::now();
    std::string filename = getLogFileName(m_basename, now);
    m_fp = ::fopen(filename.c_str(), "ae");
    m_writtenBytes = 0;
    m_lastRoll = now;
}