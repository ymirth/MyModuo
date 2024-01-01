#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include"callback.h"

#include<functional>
#include<memory>
#include<sys/epoll.h>
#include<unistd.h>
#include<string>

#include"buffer.h"
#include"channel.h"
#include"httprequestparser.h"

class EventLoop;
class Address;


class TcpConnection: public std::enable_shared_from_this<TcpConnection>
{
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
public:
    enum class ConnectionState { 
    kConnecting, 
    kConnected, 
    kDisconnecting, 
    kDisconnected };

    TcpConnection(EventLoop *loop, int conn_fd, int conn_id);   // create channel
    ~TcpConnection();// close fd

    #pragma region delete copy and move
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    TcpConnection(TcpConnection&&) = delete;
    TcpConnection& operator=(TcpConnection&&) = delete;
    #pragma endregion delete copy and move

    #pragma region set callback
    void setConnectionCallback(const ConnectionCallback &cb) { m_connection_callback = cb; }
    void setConnectionCallback(ConnectionCallback &&cb) { m_connection_callback = std::move(cb); }  // move
    void setMessageCallback(const MessageCallback &cb) { m_message_callback = cb; }
    void setMessageCallback(MessageCallback &&cb) { m_message_callback = std::move(cb); }  // move  
    void setCloseCallback(const CloseCallback &cb) { m_close_callback = cb; }
    void setCloseCallback(CloseCallback &&cb) { m_close_callback = std::move(cb); }  // move
    #pragma endregion set callback

    // Must be called in loop thread
    // call ConnectionCallback
    void connectionEstablished();   // enable reading and add to poller
    void connectionDestructor();    // disable all and remove from poller

    // Callback for m_channel
    // must be called in loop thread
    // m_channel->handleEvent() -> m_channel->handleEventWithGuard()
    // use BUFFER to read and write
    void handleRead();    // fd -> buffer ; call MessageCallback (send() -> handleWrite())
    void handleWrite();   // buffer -> fd ; if buffer is empty, disable writing
    void handleError();
    void handleClose();   // handleClose() -> connectionDestructor()

    // Thread safe api: can be called in other threads
    void shutDown(); 
    void shutDownInLoop();  // must be called in loop thread

    // Thread safe api: can be called in other threads
    void forceClose();
    void forceCloseInLoop();// must be called in loop thread
 
    // Thread safe api: can be called in other threads
    // store data in output buffer if cannot send all data 
    // Async & Nonblocking: enable writing for handleWrite() to deal with left data
    void send(const std::string &message);
    void send(const char *message, int len);
    void send(Buffer *message);
    void sendInLoop(const char *message, int len); // must be called in loop thread
   
    // void UpdateTimestamp(Timestamp now) { timestamp_ = now; }

    // Const Member Function
    // Timestamp timestamp() const { return timestamp_; }
    int getFd() const { return m_conn_fd; }
    int getId() const { return m_conn_id; }
    EventLoop* getLoop() const { return m_loop; }
    int getErrno() const{ return errno; }
    HttpRequestParser* getHttpRequestParser() { return &m_parser; }

    bool isShutdown() const { return m_state == ConnectionState::kDisconnecting; }
    bool isConnected() const { return m_state == ConnectionState::kConnected; }
    bool isDisconnected() const { return m_state == ConnectionState::kDisconnected; }
    bool isConnecting() const { return m_state == ConnectionState::kConnecting; }
    // const std::string& getIpPort() const { return m_ipport; }
    // HttpContent* GetHttpContent() { return &content_; }
private:
    ConnectionCallback m_connection_callback;
    MessageCallback m_message_callback;
    CloseCallback m_close_callback;
private:
    EventLoop *m_loop;
    int m_conn_fd;
    int m_conn_id;
    ConnectionState m_state;
    std::unique_ptr<Channel> m_channel;
    Buffer m_input_buffer;
    Buffer m_output_buffer;
    HttpRequestParser m_parser;

    // httpcontent
    // timestamp
};

#endif