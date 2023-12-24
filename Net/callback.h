#ifndef CALLBACK_H
#define CALLBACK_H

#include <functional>
#include <memory>

class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

class Buffer;
typedef std::function<void(const TcpConnectionPtr &, Buffer *)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr &, Buffer *)> MessageCallback;
typedef std::function<void()> ReadCallback;
typedef std::function<void()> WriteCallback;
typedef std::function<void(const TcpConnectionPtr &)> CloseCallback;

#endif