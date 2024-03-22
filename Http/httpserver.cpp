#include "httpserver.h"
#include "httpstate.h"
#include "httprequestparser.h"
#include "logging.h"
#include <iostream>
using std::cout;
HttpServer::HttpServer(EventLoop *loop, const Address &address, bool close_flag)
    : m_loop(loop),
      m_server(new TcpServer(loop, address)),
      m_auto_close_idle_connection(close_flag)
{
    m_server->setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1, std::placeholders::_2));
    m_server->setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    this->setResponseCallback(std::bind(&HttpServer::defaultHttpCallback, this, std::placeholders::_1, std::placeholders::_2));
    LOG_INFO << "Httpserver listening on " << address.ip() << ':' << address.port() <<"\n";
}   

HttpServer::~HttpServer()
{
}

void HttpServer::start()
{
    m_server->Start();
}

void HttpServer::defaultHttpCallback(const HttpRequest &request, HttpResponse *response)
{
    response->setStatusCode(HttpStatusCode::k404NotFound);
    response->setStatusMessage("Not Found");
    response->setCloseConnection(true);
}

void HttpServer::onConnection(const TcpConnectionPtr &conn, Buffer *buf)
{
    if (conn->isConnected())
    {
        if(m_auto_close_idle_connection)
        {
            m_loop->runAfter(kIdleConnectionTimeOuts, std::bind(&HttpServer::handleIdleConnection, this, conn));
        }
        //cout << "new connection comes and id is " << conn->getId() << std::endl;
        buf->retrieveAll();
    }
    else
    {
        using std::cout;
        // cout << "connection " << conn->getId() << " is down" << std::endl;
        buf->retrieveAll();
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf)
{// buf = conn's m_input_buffer
    auto connectionParser = conn->getHttpRequestParser();
    if(m_auto_close_idle_connection)
    {
        conn->updateLastReceiveTime();
        // std::cout<<conn->getLastReceiveTime().toString()<<std::endl;
    }
    // auto str = buf->retrieveAllAsString();
    // std::cout<<str;
    auto parser_state = connectionParser->parseContent(buf);
    buf->retrieveAll();
    if (parser_state == HttpRequestParseState::kParseErrno)
    {
        // cout << "parse error" << std::endl;
        LOG_ERROR << "parse error for connection " << conn->getId() << " close connection\n";
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutDown();
    }
    else if (parser_state == HttpRequestParseState::kParseGotCompleteRequest)
    {
        auto request = connectionParser->getRequest();
        handleRequest(conn, request);
    }
}

static int sendtimes = 0;
void HttpServer::handleRequest(const TcpConnectionPtr &conn, const HttpRequest &request)
{
    bool close = request.header("Connection") == "close" ;
    if(request.version() == HttpRequest::Version::kHttp11)
    {
        close = request.header("Connection") != "Keep-Alive";
    }
    // std::cout<<"close is "<<close<<std::endl;
    HttpResponse response(close);
    m_response_callback(request, &response);
    
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    // std::cout<<"send times is "<<++sendtimes<<std::endl;
    if (response.closeConnection())
    {
        // resquest close from client
        LOG_DEBUG << "close connection by client\n";
        conn->shutDown();
    }
}


void HttpServer::handleIdleConnection(const std::weak_ptr<TcpConnection> &conn)
{
    // std::cout<<"handleIdleConnection\n";
    auto connection = conn.lock();
    if (connection)
    {   
        auto time1 = Timestamp::addTime(connection->getLastReceiveTime(), kIdleConnectionTimeOuts);
        LOG_DEBUG << connection->getLastReceiveTime().toString() << "\n"<< time1.toString() << "\n";
        auto time2 = Timestamp::now();
        if(Timestamp::addTime(connection->getLastReceiveTime(), kIdleConnectionTimeOuts) < Timestamp::now())
        {
            LOG_INFO << "Connection : " << connection->getId() << " is Idle For a Long Time : " << kIdleConnectionTimeOuts << "s\n";
            connection->shutDown();
        }
        else
        {
            m_loop->runAfter(kIdleConnectionTimeOuts, std::bind(&HttpServer::handleIdleConnection, this, conn));
        }
    }
}