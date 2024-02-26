#include "tcpserver.h"
#include "address.h"
#include "callback.h"
#include "tcpconnection.h"
#include "eventloop.h"

#include "httprequest.h"
#include "httprequestparser.h"
#include "httpresponse.h"
#include "httpstate.h"
#include "httpresponsefile.h"
#include "httpserver.h"

#include "logging.h"
#include "asynclogging.h"

#include <iostream>
#include <memory>
using std::cout;

#include <unistd.h>

// log config
std::unique_ptr<AsyncLogging> asynclog;
extern void setOutput(Logger::OutputFunc);
extern void setFlush(Logger::FlushFunc);

void AsyncOutputFunc(const char* data, int len) {
  asynclog->append(data, len);
}

void AsyncFlushFunc() {
  asynclog->flush();
}

// onMessage
void resCallback(const HttpRequest &request, HttpResponse *resp)
{
    auto &response = *resp;
    if (request.method() != HttpRequest::Method::kGet)
    {
        response.setStatusCode(HttpStatusCode::k400BadRequest);
        response.setStatusMessage("Bad Request");
        response.setCloseConnection(true);
        return;
    }
    // cout << "request path is " << request.path() << std::endl;

    {
        const string &path = request.path();
        if (path == "/")
        {
            response.setStatusCode(HttpStatusCode::k200OK);
            response.setStatusMessage("OK");
            response.setContentType("text/html");
            response.setBody(love6_website);
        }
        else if (path == "/hello")
        {
            response.setStatusCode(HttpStatusCode::k200OK);
            response.setStatusMessage("OK");
            response.setContentType("text/html");
            response.setBody("Hello, world!\n");
        }
        else if (path == "/favicon.ico" || path == "/favicon")
        {
            response.setStatusCode(HttpStatusCode::k200OK);
            response.setStatusMessage("OK");
            response.setContentType("image/png");
            response.setBody(string(favicon, sizeof(favicon)));
        }
        else
        {
            response.setStatusCode(HttpStatusCode::k404NotFound);
            response.setStatusMessage("Not Found");
            response.setCloseConnection(true);
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    //log config
    asynclog.reset(new AsyncLogging("./log"));
    setOutput(AsyncOutputFunc);
    setFlush(AsyncFlushFunc);
    asynclog->startAsyncLogging();

    // NOTE : for benchmark
    // SetLogLevel(Logger::Level::ERROR);

    EventLoop loop;
    Address address("0.0.0.0", 9090);
    HttpServer server(&loop, address);
    cout<<std::thread::hardware_concurrency()<<std::endl;
    server.setThreadNum(std::thread::hardware_concurrency()+1);
    server.setResponseCallback(resCallback);
    server.start();
    loop.loop();
    return 0;
}