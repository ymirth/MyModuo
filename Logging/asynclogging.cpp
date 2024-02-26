#include "asynclogging.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>
#include <string>

// 原理解析：http://jinke.me/2018-05-10-muduo-logger/

void AsyncLogging::append(const char *data, int len)
{   // 写入数据到前端缓冲区
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentBuffer->avail() > len)
    {
        m_currentBuffer->append(data, len);
    }
    else
    {
        m_buffers.push_back(std::move(m_currentBuffer));
        if (m_nextBuffer)
        {
            m_currentBuffer = std::move(m_nextBuffer);  // 会导致!m_nextBuffer == true
        }
        else
        {
            m_currentBuffer.reset(new Buffer);
        }
        m_currentBuffer->append(data, len);
        m_cond.notify_one();
    }
}

void AsyncLogging::flush()
{
   fflush(stdout);
}

void AsyncLogging::threadFunc()
{
    LogFilePtr logFile(new LogFile(m_filepath, kRollSize));
    // 后端缓冲区，用于归还前端得缓冲区，currentBuffer nextBuffer
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->setZero();
    newBuffer2->setZero();
    // 后端缓冲区数组
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (m_running)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_buffers.empty())
            {
                // 等待3秒也会解除阻塞
                m_cond.wait_for(lock, std::chrono::seconds((int)kFlushInterval));
            }
            // 此时正使用得buffer也放入buffer数组中（没写完也放进去，避免等待太久才刷新一次）
            m_buffers.emplace_back(std::move(m_currentBuffer));
            // 归还正在使用的缓存区
            m_currentBuffer = std::move(newBuffer1);
            // 后端缓存区与前端缓存区交换
            buffersToWrite.swap(m_buffers);
            if (!m_nextBuffer)
            {
                m_nextBuffer = std::move(newBuffer2);
            }
        }
        // if (buffersToWrite.size() > 25)
        // {
        //     buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        // }
        for (const auto &buffer : buffersToWrite)
        {
            logFile->append(buffer->data(), buffer->len());
            // lofFile automatically flush or roll if necessary
        }
        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }
        if (!newBuffer1)
        {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        if (!newBuffer2)
        {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        buffersToWrite.clear();
        logFile->flush();
    }
    logFile->flush();
}


