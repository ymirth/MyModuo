#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <memory>
#include <functional>
#include <string>
#include <unordered_map>

#include "callback.h"
#include "eventloop.h"
#include "address.h"
#include "tcpconnection.h"
#include "tcpserver.h"

#include "httprequest.h"
#include "httprequestparser.h"
#include "httpresponse.h"

class HttpServer
{
    using HttpResponseCallback = std::function<void(const HttpRequest &, HttpResponse *)>;

public:
    HttpServer(EventLoop *loop, const Address &addr, bool close_flag = false);
    ~HttpServer();

    void start();

    void defaultHttpCallback(const HttpRequest &request, HttpResponse *response);

    void setThreadNum(int num_threads) { m_server->setThreadNum(num_threads); }
    void setResponseCallback(const HttpResponseCallback &cb) { m_response_callback = cb; }
    void setResponseCallback(HttpResponseCallback &&cb) { m_response_callback = std::move(cb); }

    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn, Buffer *buffer);

    void handleRequest(const TcpConnectionPtr &conn, const HttpRequest &request);
private:
    EventLoop *m_loop;
    std::unique_ptr<TcpServer> m_server;
    bool m_auto_close_idle_connection;
    HttpResponseCallback m_response_callback;
};

#endif // HTTP_SERVER_H