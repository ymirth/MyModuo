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

#include <iostream>
using std::cout;

#include <unistd.h>

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
    cout << "request path is " << request.path() << std::endl;

    // response.setStatusCode(HttpStatusCode::k200OK);
    // response.setStatusMessage("OK");
    // response.setCloseConnection(false);
    // response.setContentType("text/html");
    // response.setBody("<html><head><title>This is title</title></head>"
    //                  "<body><h1>Hello</h1>Now is "
    //                  + std::to_string(time(NULL)) +
    //                  "</body></html>");

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
            response.setStatusCode(HttpStatusCode::k200OK);
            response.setStatusMessage("Not Found");
            response.setCloseConnection(true);
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    EventLoop loop;
    Address address("0.0.0.0", 9090);
    HttpServer server(&loop, address);
    server.setThreadNum(4);
    server.setResponseCallback(resCallback);
    server.start();
    loop.loop();
    return 0;
}