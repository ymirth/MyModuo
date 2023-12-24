#include "eventloop.h"
#include "address.h"

#include "tcpconnection.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>

TcpConnection::TcpConnection(EventLoop *loop, int conn_fd, int conn_id)
    : m_loop(loop),
      m_conn_fd(conn_fd),
      m_conn_id(conn_id),
      m_channel(new Channel(loop, conn_fd)),
      m_state(ConnectionState::kConnecting)
{
    m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
}

TcpConnection::~TcpConnection()
{
    ::close(m_conn_fd);
}

void TcpConnection::connectionEstablished()
{
    m_loop->assertInLoopThread();
    m_state = ConnectionState::kConnected;
    m_channel->Tie(shared_from_this()); // tie channel and TcpConnection； 1.weak_ptr 2.weak_ptr.lock()
    m_channel->enableReading();
    m_connection_callback(shared_from_this(), &m_input_buffer);
}

void TcpConnection::connectionDestructor()
{
    m_loop->assertInLoopThread();
    if (m_state == ConnectionState::kConnected || m_state == ConnectionState::kDisconnecting)
    {
        m_state = ConnectionState::kDisconnected;
        m_channel->disableAll();
        m_connection_callback(shared_from_this(), &m_input_buffer);
    }
    m_loop->remove(m_channel.get());
}

void TcpConnection::handleRead()
{
    m_loop->assertInLoopThread();
    //int saved_errno = 0;
    ssize_t n = m_input_buffer.readFd(m_conn_fd);
    if (n > 0)
    {
        m_message_callback(shared_from_this(), &m_input_buffer);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        //errno = saved_errno;
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    m_loop->assertInLoopThread();
    if (m_channel->isWriting())
    {
        // write作用： 将output_buffer_中的数据写入fd_中
        ssize_t n = ::write(m_conn_fd, m_output_buffer.peek(), m_output_buffer.readableBytes());
        if (n > 0)
        {
            m_output_buffer.retrieve(n);
            if (m_output_buffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if (m_state == ConnectionState::kDisconnecting)
                {
                    shutDownInLoop();
                }
            }
        }
    }
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    int err = ::getsockopt(m_conn_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (err < 0)
    {
        // log error
    }
}

void TcpConnection::handleClose()
{
    m_loop->assertInLoopThread();
    assert(m_state == ConnectionState::kConnected || m_state == ConnectionState::kDisconnecting);
    m_state = ConnectionState::kDisconnected;
    m_channel->disableAll();
    
    m_close_callback(shared_from_this());
}


void TcpConnection::shutDown()
{
    if (m_state == ConnectionState::kConnected)
    {
        m_state = ConnectionState::kDisconnecting;
        m_loop->runInLoop(std::bind(&TcpConnection::shutDownInLoop, this));
    }
}

void TcpConnection::shutDownInLoop()
{
    m_loop->assertInLoopThread();
    if (!m_channel->isWriting())
    {
        ::shutdown(m_conn_fd, SHUT_WR);
    }
}

void TcpConnection::forceClose()
{
    if (m_state == ConnectionState::kConnected || m_state == ConnectionState::kDisconnecting)
    {
        m_state = ConnectionState::kDisconnecting;
        m_loop->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, this));
    }
}

void TcpConnection::forceCloseInLoop()
{
    m_loop->assertInLoopThread();
    if (m_state == ConnectionState::kConnected || m_state == ConnectionState::kDisconnecting)
    {
        handleClose();
    }
}

void TcpConnection::send(const std::string &message)
{
    this->send(message.c_str(), message.size());
}

void TcpConnection::send(const char *message, int len)
{
    if (m_state == ConnectionState::kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(message, len);
        }
        else
        {
            m_loop->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message, len));
        }
    }
}

void TcpConnection::send(Buffer *message)
{
    if (m_state == ConnectionState::kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(message->peek(), message->readableBytes());
            message->retrieveAll();
        }
        else
        {
            std::function<void()> func = std::bind(&TcpConnection::sendInLoop, this, message->peek(), message->readableBytes());
            m_loop->runInLoop(func);
            message->retrieveAll();
        }
    }
}



void TcpConnection::sendInLoop(const char* msg, int len)
{
    m_loop->assertInLoopThread();
    ssize_t wroteNum = 0;
    size_t remaining = len;
    bool faultError = false;
    if (m_state == ConnectionState::kDisconnected)
    {
        // log error
        return;
    }
    if (!m_channel->isWriting() && m_output_buffer.readableBytes() == 0)
    {
        wroteNum = ::write(m_conn_fd, msg, len);
        if (wroteNum >= 0)
        {
            remaining = len - wroteNum;
        }
        else
        {
            wroteNum = 0;
            if (errno != EWOULDBLOCK) // EWOULDBLOCK: no buffer space available
            {
                // log error
                if (errno == EPIPE || errno == ECONNRESET) // EPIPE: Broken pipe; ECONNRESET: Connection reset by peer
                {
                    faultError = true;
                }
            }
        }
    }
    assert(remaining <= len);
    if (!faultError && remaining > 0)
    {
        m_output_buffer.append(msg + wroteNum, remaining);
        if (!m_channel->isWriting()) // if not writing, enable writing: for handleWrite() to deal with left data
        {
            m_channel->enableWriting();
        }
    }   
}