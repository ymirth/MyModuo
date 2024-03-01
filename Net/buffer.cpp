#include"buffer.h"
#include"logging.h"
#include<sys/uio.h>

int Buffer::readFd(int fd)
{
    char extrabuf[64 * 1024];
    struct iovec vec[2];  // 两个缓冲区
    const int writeable = writeableBytes();
    vec[0].iov_base = curWritePtr();
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const int iovcnt = (writeable < sizeof(extrabuf)) ? 2 : 1;
    // readv: 从fd中读取数据到vec中
    const int n = ::readv(fd, vec, iovcnt);
    if(n < 0)
    {
        //log errors
        LOG_ERROR<<"readv error\n";
        return -1;
    }
    else if(n <= writeable)
    {
        m_write_index += n;
    }
    else
    {
        m_write_index = m_buffer.size();
        append(extrabuf, n - writeable);
    }
    return n;
}