#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>

#include <assert.h>
#include <cstring>
#include <atomic>
#include <algorithm>

using std::string;

static const int kPrePendIndex = 8; //
static const int kInitialSize = 1024;
static const char* kCRLF = "\r\n";  // CRLF: Carriage Return and Line Feed

class Buffer{
private:
    std::vector<char> m_buffer;
    std::atomic<int> m_read_index;    // read index
    std::atomic<int> m_write_index;   // write index
public:
    Buffer(int init_buffer_size = 1024) : m_buffer(init_buffer_size), m_read_index(0), m_write_index(0) {}
    ~Buffer() = default;

    #pragma region delete copy and move
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(Buffer&&) = delete;
    #pragma endregion delete copy and move
    
    // 获取buffer迭代器
    char* begin() {return &*m_buffer.begin();} 
    const char* begin() const {return &*m_buffer.begin();}

    // 获取缓存剩余字节
    int readableBytes() const { return m_write_index - m_read_index; }
    int writeableBytes() const { return static_cast<int>(m_buffer.size()) - m_write_index; }
    int prependableBytes() const { return m_read_index; }
    
    // 获取读写指针
    char* curReadPtr() { return begin() + m_read_index; }
    const char* curReadPtr() const { return begin() + m_read_index; }
    char* curWritePtr() { return begin() + m_write_index; }
    const char* curWritePtr() const { return begin() + m_write_index; }

    // 读写指针移动
    void moveReadIndex(int len) { m_read_index += len; }
    void moveWriteIndex(int len) { m_write_index += len; }

    // 写入数据
    void append(const char* data, int len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, curWritePtr());
        moveWriteIndex(len);
    }
    void append(const char* msg)
    {
        append(msg, static_cast<int>(strlen(msg)));
    }
    void append(const string& msg)
    {
        append(msg.c_str(), static_cast<int>(msg.size()));
    }

    // 读取数据后，从缓存中删除数据：移动读指针
    void retrieveUntilIndex(const char* index)
    {
        assert(curReadPtr() <= index);
        assert(index <= curWritePtr());
        retrieve(index - curReadPtr());
    }
    void retrieve(int len)
    {
        assert(len <= readableBytes());
        if(len < readableBytes())
        {
            m_read_index += len;
        }
        else
        {
            retrieveAll();
        }
    }
    void retrieveAll()
    {
        m_read_index = kPrePendIndex;
        m_write_index = kPrePendIndex;
    }

    // 确保缓存有足够的空间, 确保缓存 可写空间 >= len
    void ensureWriteableBytes(int len)
    {
        if(writeableBytes() >= len) return;
        if(writeableBytes() + prependableBytes() >= len + kPrePendIndex)
        {   // (可写空间 + 预留空间 - 头部空间) ==  缓存空闲空间  >= len      
            // 移动数据 --> 到缓存头部
            // std::copy(curReadPtr(), curWritePtr(), begin() + kPrePendIndex);
            m_write_index = kPrePendIndex + readableBytes();
            m_read_index = kPrePendIndex;
        }
        else // 实际的可写空间 < len
        {
            m_buffer.resize(m_write_index + len);
        }
    }

    // 查找CRLF
    const char* findCRLF() const
    {
        const char* crlf = std::search(curReadPtr(), curWritePtr(), kCRLF, kCRLF + 2);
        return crlf == curWritePtr() ? nullptr : crlf;
    }

    // buffer peek: 返回缓存中的数据
    char* peek(){ return curReadPtr(); }   
    const char* peek() const { return curReadPtr(); }
 
    // 外部接口
    string retrieveAllAsString()
    {// move 语义，是的string对象不会发生拷贝
        string str(curReadPtr(), readableBytes());
        retrieveAll();
        return str;
    }
    
    // 读取fd中的数据到缓存中
    int readFd(int fd); 
};

#endif