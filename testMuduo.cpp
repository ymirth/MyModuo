#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <boost/bind.hpp>
#include <muduo/net/EventLoop.h>
#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <iostream>

// 使用muduo开发回显服务器
class EchoServer
{
 public:
  EchoServer(muduo::net::EventLoop* loop,
             const muduo::net::InetAddress& listenAddr);

  void start(); 

 private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);

  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp time);

  muduo::net::TcpServer server_;
};

EchoServer::EchoServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr)
  : server_(loop, listenAddr, "EchoServer")
{
  server_.setConnectionCallback(
      boost::bind(&EchoServer::onConnection, this, _1));
  server_.setMessageCallback(
      boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start()
{
  server_.start();
}

void EchoServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
  LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
}

void EchoServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)
{
  // 接收到所有的消息，然后回显
  muduo::string msg(buf->retrieveAllAsString());
  LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
           << "data received at " << time.toString();
  conn->send(msg);
}




// using namespace muduo;
// using namespace muduo::net;
const std::string website = R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Ymirth - Personal Homepage</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 20px;
                    padding: 20px;
                    text-align: center;
                }

                header {
                    background-color: #333;
                    color: white;
                    padding: 10px;
                }

                h1 {
                    margin-bottom: 0;
                }

                nav {
                    margin-top: 20px;
                }

                nav a {
                    text-decoration: none;
                    color: #333;
                    padding: 10px;
                    margin: 0 10px;
                    border-radius: 5px;
                    background-color: #eee;
                }

                section {
                    margin-top: 20px;
                    text-align: left;
                }

                footer {
                    margin-top: 20px;
                    background-color: #333;
                    color: white;
                    padding: 10px;
                }
            </style>
        </head>
        <body>
            <header>
                <h1>Ymirth</h1>
                <p>Software Developer</p>
            </header>

            <nav>
                <a href="#about">About Me</a>
                <a href="#skills">Skills</a>
                <a href="#projects">Projects</a>
                <a href="#contact">Contact</a>
            </nav>

            <section id="about">
                <h2>About Me</h2>
                <p>I am a student major in cs in scut</p>
                <p>I’m currently learning distributed system and database</p>
                <p>I’m looking to collaborate on backend program</p>
            </section>

            <section id="skills">
                <h2>Skills</h2>
                <ul>
                    <li>Cpp, Python, Go</li>
                    <li>Linux, Gdb, Network Programing, Concurency Programming</li>
                    <li>Mysql, Redis</li>
                </ul>
            </section>

            <section id="projects">
                <h2>Projects</h2>
                <p>Threadpool in cpp14:<a href="https://github.com/ymirth/ThreadPool" target="_blank">/github.com/ymirth/ThreadPool</a></p>
                <p>HTTP Server Frame Work based on Moduo:<a href="https://github.com/ymirth/MyModuo" target="_blank">/github.com/ymirth/webserver</a></p>
            </section>

            <section id="contact">
                <h2>Contact</h2>
                <p>Email: ymirth123@gmail.com</p>
                <p>GitHub: <a href="https://github.com/ymirth" target="_blank">github.com/ymirth</a></p>
            </section>

            <footer>
                &copy; 2024 Ymirth
            </footer>
        </body>
        </html>
    )";

void onRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse* resp)
{
    std::string query = req.query();
    resp->setStatusCode(muduo::net::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/html");
    resp->setCloseConnection(false);
    resp->setBody(website);   //"query string: " + query + "\n");
    // std::cout<<"hello\n";
}



int main()
{
    LOG_INFO << "pid = " << getpid();
    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr("0.0.0.0",1234);
//   EchoServer server(&loop, listenAddr);
    muduo::net::HttpServer server(&loop, listenAddr, "MyHTTPServer");
    server.setThreadNum(3);
    server.setHttpCallback(onRequest);
    server.start();

//   server.start();
  loop.loop();
}
