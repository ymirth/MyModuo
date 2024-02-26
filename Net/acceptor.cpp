#include<fcntl.h>
#include<sys/types.h>
#include<netinet/tcp.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>

#include<functional>
#include<iostream>

#include "channel.h"
#include "eventloop.h"
#include "address.h"
#include "acceptor.h"
#include "logging.h"

static void Bind(int listen_fd, const Address &address)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.port());
    addr.sin_addr.s_addr = inet_addr(address.ip());
    int ret = bind(listen_fd, (struct sockaddr*)& addr, sizeof(addr));
    if(ret < 0)
    {
        perror("bind");
        exit(1);
    }
}



static void setReuseAddr(int listen_fd)
{// reuse address
    int opt = 1;
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(ret < 0)
    {
        perror("setsockopt");
        exit(1);
    }
}

static void setKeepAlive(int listen_fd)
{// enable TCP keepalive
    int opt = 1;
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    if(ret < 0)
    {
        perror("setsockopt");
        exit(1);
    }
}

static void setTcpNoDelay(int listen_fd)
{// disable Nagle's algorithm : send data immediately
    int opt = 1;
    int ret = setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    if(ret < 0)
    {
        perror("setsockopt");
        exit(1);
    }
}


void Acceptor::Listen()
{
    int ret = listen(m_listen_fd, SOMAXCONN);
    assert(ret != -1);
    m_accept_channel->enableReading();
}


/*
    在这个上下文中，idlefd_被用作一个备用的文件描述符，
    当系统达到最大文件描述符限制时，可以关闭这个文件描述符以便打开新的文件或套接字。
    这是一种常见的技巧，用于处理"too many open files"的错误。
*/
Acceptor::Acceptor(EventLoop *loop, const Address &address) : 
    // non-blocking and close-on-exec (新的进程不会继承该文件描述符) and TCP No Delay
    m_listen_fd(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
    m_idle_fd(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
    m_address(address), m_loop(loop), 
    m_accept_channel(new Channel(m_loop, m_listen_fd))
{
    if(m_listen_fd < 0)
    {
        perror("socket");
        exit(1);
    }
    setReuseAddr(m_listen_fd);
    Bind(m_listen_fd, m_address);
    // Listen(m_listen_fd);

    m_accept_channel->setReadCallback(std::bind(&Acceptor::handleConnect, this));
    m_accept_channel->setIndex(ChannelIndex::kNew);
    m_accept_channel->setEvents(EPOLLIN | EPOLLET);
}

Acceptor::~Acceptor()
{
    // m_accept_channel->disableAll();
    // m_loop->remove(m_accept_channel.get());
    ::close(m_listen_fd);
    ::close(m_idle_fd);
}

void Acceptor::handleConnect()
{
    struct sockaddr_in address;
    socklen_t len = sizeof(address);
    // SOCK_NONBLOCK | SOCK_CLOEXEC : set non-blocking and close-on-exec (新的进程不会继承该文件描述符)
    int conn_fd = accept4(m_listen_fd, (struct sockaddr*)&address, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(conn_fd < 0)
    {
        if(errno == EMFILE)  // EMFILE: too many open files
        {
            LOG_ERROR << "acceptor: too many open files\n";
            close(m_idle_fd);
            m_idle_fd = accept(m_listen_fd, nullptr, nullptr);
            close(m_idle_fd);
            m_idle_fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
        return;
    }
    setKeepAlive(conn_fd);
    setTcpNoDelay(conn_fd);
    if(m_new_connection_callback)
    {
        // sockaddr_in -> Address(char* ip, int port)
        // address.sin_addr.s_addr -> ip
        // address.sin_port -> port
        auto ip = inet_ntoa(address.sin_addr);
        auto port = ntohs(address.sin_port);
        Address clientAddr(ip, port);
        m_new_connection_callback(conn_fd, clientAddr);
    }
    // close(conn_fd);
}



