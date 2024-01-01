#include "httpserver.h"
#include "httpstate.h"
#include "httprequestparser.h"
#include <iostream>
using std::cout;
HttpServer::HttpServer(EventLoop *loop, const Address &address, bool close_flag)
    : m_loop(loop),
      m_server(new TcpServer(loop, address)),
      m_auto_close_idle_connection(close_flag)
{
    m_server->setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    m_server->setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    this->setResponseCallback(std::bind(&HttpServer::defaultHttpCallback, this, std::placeholders::_1, std::placeholders::_2));
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

void HttpServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->isConnected())
    {
        cout << "new connection comes and id is " << conn->getId() << std::endl;
    }
    else
    {
        cout << "connection " << conn->getId() << " is down" << std::endl;
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf)
{
    auto connectionParser = conn->getHttpRequestParser();
    auto parser_state = connectionParser->parseContent(buf);
    if (parser_state == HttpRequestParseState::kParseErrno)
    {
        cout << "parse error" << std::endl;
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutDown();
    }
    else if (parser_state == HttpRequestParseState::kParseGotCompleteRequest)
    {
        auto request = connectionParser->getRequest();
        handleRequest(conn, request);
    }
}

void HttpServer::handleRequest(const TcpConnectionPtr &conn, const HttpRequest &request)
{
    bool close = false;
    // //request.header("Connection") == "close" ||
    //              (request.version() == HttpRequest::Version::kHttp11 
    //              && request.header("Connection") != "Keep-Alive")

    HttpResponse response(close);
    m_response_callback(request, &response);
    
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if (response.closeConnection())
    {
        conn->shutDown();
    }
}