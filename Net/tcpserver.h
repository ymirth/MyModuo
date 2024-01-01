#ifndef TCPSEVER_H
#define TCPSEVER_H

#include"callback.h"

#include<functional>
#include<memory>
#include<string>
#include<map>

class EventLoop;
class Address;
class TcpConnection;
class Acceptor;
class LoopThreadPool;
// class threadpool;

class TcpServer{
public:
    TcpServer(EventLoop *loop, const Address &listen_addr);
    ~TcpServer();

    void Start() ;
    void setThreadNum(int num_threads);
    #pragma region delete copy and move
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;
    #pragma endregion delete copy and move

    #pragma region set callback
    void setConnectionCallback(const ConnectionCallback &cb) { m_connection_callback = cb; }
    void setConnectionCallback(ConnectionCallback &&cb) { m_connection_callback = std::move(cb); }  // move
    void setMessageCallback(const MessageCallback &cb) { m_message_callback = cb; }
    void setMessageCallback(MessageCallback &&cb) { m_message_callback = std::move(cb); }  // move
    // void setCloseCallback(const CloseCallback &cb) { m_close_callback = cb; }
    // void setCloseCallback(CloseCallback &&cb) { m_close_callback = std::move(cb); }  // move
    #pragma endregion set callback

    void handleNewConnection(int conn_fd, const Address &con_addr);
    void handleRemoveConnection(const TcpConnectionPtr &conn);
    void handleRemoveConnectionInLoop(const TcpConnectionPtr &conn);

private:
    EventLoop *m_loop;
    int m_next_connection_id;
    std::unique_ptr<Acceptor> m_acceptor;
    std::unique_ptr<LoopThreadPool> m_loop_thread_pool;           // IO thread pool
    // std::unique_ptr<threadpool> m_worker_thread_pool;                    // thread pool for compute
    std::map<int, std::shared_ptr<TcpConnection>> m_connections;  // fd -> TcpConnection
    
    
    // info for connection
    const std::string m_ipport;

    ConnectionCallback m_connection_callback;
    MessageCallback m_message_callback;
};


#endif