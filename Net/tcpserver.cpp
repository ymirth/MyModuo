#include"channel.h"
#include"epoller.h"
#include"eventloop.h"
#include"address.h"
#include"acceptor.h"
#include"buffer.h"
#include"tcpconnection.h"
#include"loopthreadpool.h"

#include"logging.h"

#include"tcpserver.h"

#include<string>
#include<memory>
#include<functional>

TcpServer::TcpServer(EventLoop *loop, const Address &address)
    :m_loop(loop),
    m_loop_thread_pool(new LoopThreadPool(loop)),
    m_acceptor(new Acceptor(loop, address)),
    m_next_connection_id(0),
    m_ipport(std::string(address.ip()) + ":" + std::to_string(address.port()))
{
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::handleNewConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto& item : m_connections)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectionDestructor, conn));
    }
}

void TcpServer::Start()
{
    m_loop_thread_pool->start();
    m_acceptor->Listen();
}

void TcpServer::setThreadNum(int num_threads)
{
    m_loop_thread_pool->setThreadNum(num_threads);
}

void TcpServer::handleNewConnection(int conn_fd, const Address &con_addr)
{
    EventLoop *loop = m_loop_thread_pool->getNextLoop();
    TcpConnectionPtr ptr = std::make_shared<TcpConnection>(loop, conn_fd, m_next_connection_id++);
    m_connections[conn_fd] = ptr;

    if(m_next_connection_id == std::numeric_limits<int>::max()) m_next_connection_id = 0;  
    
   // log info by cout
    std::string conn_name = m_ipport + " #" + std::to_string(m_next_connection_id);
    // std:: << "New connection: " << conn_name << " from " << con_addr.ip() << ":" << con_addr.port() << std::endl;
    LOG_INFO << "New connection: " << conn_name << " from " << con_addr.ip() << ":" << con_addr.port()<<"\n";
    ptr->setIPPort(con_addr.ip(), con_addr.port());

    // set callback: connection/message/close
    ptr->setConnectionCallback(m_connection_callback);
    ptr->setMessageCallback(m_message_callback);
    ptr->setCloseCallback(std::bind(&TcpServer::handleRemoveConnection, this, std::placeholders::_1));

    loop->runInLoop(std::bind(&TcpConnection::connectionEstablished, ptr));
}

void TcpServer::handleRemoveConnection(const TcpConnectionPtr &conn)
{
    m_loop->runInLoop(std::bind(&TcpServer::handleRemoveConnectionInLoop, this, conn));
}

void TcpServer::handleRemoveConnectionInLoop(const TcpConnectionPtr &conn)
{
    m_loop->assertInLoopThread(); // must be called in loop thread
    // 1. m_connections is not thread safe 2. m_connections.erase() is not thread safe
    m_connections.erase(conn->getFd());
    LOG_INFO << "TcpServer::HandleCloseInLoop - remove connection " << "[" 
           << conn->getIP() << ":" << conn->getPort() << "]\n";
    EventLoop *loop = conn->getLoop();
    loop->queueInLoop(std::bind(&TcpConnection::connectionDestructor, conn));
}
