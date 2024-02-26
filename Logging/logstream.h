#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include <string.h>

#include <string>
#include <vector>
#include <algorithm>

const int kSmallBuffer = 4096;
const int kLargeBuffer = 4096 * 1000;
static const int kMaxNumericSize = 48; // 数字最大长度
static const char digits[] = "9876543210123456789";
static const char *zero = digits + 9;

template <int SIZE>
class FixedBuffer
{
public:
#pragma region noncopyable
    FixedBuffer(const FixedBuffer &) = delete;
    FixedBuffer &operator=(const FixedBuffer &) = delete;
#pragma endregion
    FixedBuffer() : m_cur(m_data)
    {
        setCookie(cookieStart);
    }
    ~FixedBuffer()
    {
        setCookie(cookieEnd);
    }

    void setCookie(void (*cookie)()) { m_cookie = cookie; }
    std::string toString() const { return std::string(m_data, m_cur - m_data); }

    void setZero()
    {
        memset(m_data, '\0', sizeof(m_data));
        m_cur = m_data;
    }
    void reset() { m_cur = m_data; }

    void append(const char *buf, size_t len)
    {
        if (avail() > static_cast<int>(len))
        {
            std::copy(buf, buf + len, m_cur);
            m_cur += len;
        }
    }

    const char *data() const { return m_data; }
    char *current() { return m_cur; }
    const char *end() const { return m_data + sizeof(m_data); }

    void add(size_t len) { m_cur += len; }
    int len() const { return static_cast<int>(m_cur - m_data); }
    int avail() const { return static_cast<int>(end() - m_cur); }

private:
    char m_data[SIZE];       // 缓冲区
    char *m_cur;        // 写指针
    void (*m_cookie)(); // 回调函数
private:
    static void cookieStart();
    static void cookieEnd();
};

class LogStream
{
public:
    using Buffer = FixedBuffer<kSmallBuffer>;

private:
    using self = LogStream;
    mutable Buffer m_buffer;

public:
    LogStream() = default;
    ~LogStream() = default;

    Buffer &buffer() { return m_buffer; }

    template <typename T>
    void formatInteger(T num)
    {
        if (m_buffer.avail() >= kMaxNumericSize)
        { //
            char *buf = m_buffer.current();
            char *now = buf;
            const char *zero = digits + 9;
            bool negative = num < 0;

            do
            {
                int remainder = static_cast<int>(num % 10);
                *(now++) = zero[remainder];
                num /= 10;
            } while (num != 0);

            if (negative)
                *(now++) = '-';
            *now = '\0';
            std::reverse(buf, now);
            m_buffer.add(static_cast<int>(now - buf));
        }
    }

    self &operator<<(bool v)
    {
        m_buffer.append(v ? "1" : "0", 1);
        return *this;
    }

    // 调用formatInteger
    self &operator<<(int num)
    {
        formatInteger(num);
        return *this;
    }
    self &operator<<(unsigned int num)
    {
        formatInteger(num);
        return *this;
    }
    self &operator<<(short num)
    {
        return (*this) << static_cast<int>(num);
    }
    self &operator<<(unsigned short num)
    {
        return (*this) << static_cast<unsigned int>(num);
    }
    self &operator<<(long num)
    {
        formatInteger(num);
        return *this;
    }
    self &operator<<(unsigned long num)
    {
        formatInteger(num);
        return *this;
    }
    self &operator<<(long long num)
    {
        formatInteger(num);
        return *this;
    }
    self &operator<<(unsigned long long num)
    {
        formatInteger(num);
        return *this;
    }

    // 写入浮点数
    self &operator<<(double num)
    {
        if (m_buffer.avail() >= kMaxNumericSize)
        {
            int len = snprintf(m_buffer.current(), kMaxNumericSize, "%.12g", num);
            m_buffer.add(len);
        }
        return *this;
    }
    self &operator<<(float num)
    {
        *this << static_cast<double>(num);
        return *this;
    }

    // 写入字符串
    self &operator<<(const char *str)
    {
        if (str)
            m_buffer.append(str, strlen(str));
        else
            m_buffer.append("(null)", 6);
        return *this;
    }
    self &operator<<(const unsigned char *str)
    {
        return operator<<(reinterpret_cast<const char *>(str));
    }
    self &operator<<(const std::string &str)
    {
        m_buffer.append(str.c_str(), str.size());
        return *this;
    }

    // 指针准换成16进制
    self &operator<<(const void *data)
    {
        uintptr_t num = reinterpret_cast<uintptr_t>(data);
        if (m_buffer.avail() >= kMaxNumericSize)
        {
            char *buf = m_buffer.current();
            buf[0] = '0';
            buf[1] = 'x';
            int len = snprintf(buf + 2, kMaxNumericSize - 2, "%p", num);
            m_buffer.add(len + 2);
        }
        return *this;
    }
};

#endif // LOGSTREAM_H