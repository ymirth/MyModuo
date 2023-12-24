#include"tcpserver.h"
#include"address.h"
#include"callback.h"
#include"tcpconnection.h"
#include"eventloop.h"

#include<iostream>
using std::cout;

#include<unistd.h>

// callback
void onConnection(const TcpConnectionPtr & conn, Buffer *buf)
{
    if(conn->isConnected())
    {
        cout<<"new connection comes and id is "<<conn->getId()<<std::endl; 
    }
    else
    {
        cout<<"connection "<<conn->getId()<<" is down"<<std::endl;
    }
}
// message callback
void onMessage(const TcpConnectionPtr & conn, Buffer *buf)
{
    cout<<"message comes and id is :"<<conn->getId()<<std::endl;
    std::string msg = buf->retrieveAllAsString();
    cout<<"message is "<<msg<<std::endl;
    conn->send(msg);
}


int main(int argc, char *argv[])
{
    EventLoop loop;
    Address address("0.0.0.0",8000);
    TcpServer server(&loop, address);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setThreadNum(4);
    server.Start();
    loop.loop();
    return 0;
}