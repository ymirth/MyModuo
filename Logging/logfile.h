#ifndef LOGFILE_H
#define LOGFILE_H

#include <sys/time.h>
#include <sys/types.h>

#include <chrono>
#include <string>

#include"timestamp.h"


class LogFile
{
public:
    LogFile(const std::string& basename, off_t rollSize, int flushInterval = 3, bool threadSafe = true);
    ~LogFile();

    void append(const char* logline, int len);
    void flush() { fflush(m_fp);}
    void rollFile();
private:
    FILE* m_fp;
    int64_t m_writtenBytes;
    Timestamp m_lastRoll;
    Timestamp m_lastFlush;
    off_t m_rollSize;
    const int m_flushInterval;
    const std::string m_basename;

};



#endif
