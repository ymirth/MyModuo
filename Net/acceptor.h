#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include<functional>
#include<memory>

class Address;
class Channel;
class EventLoop;

class Acceptor{
private:
    int m_listen_fd;                                        // listen_fd
    int m_idle_fd;                                          // idle_fd
    Address m_address;                                      // listen address
    EventLoop *m_loop;                                      // Acceptor belongs to which EventLoop
    std::unique_ptr<Channel> m_accept_channel;              // Channel for listen_fd
    std::function<void(int, Address)> m_new_connection_callback; // callback when new connection comes
    // (int, Address) : (conn_fd, peer_addr)
public:
    Acceptor(EventLoop *loop, const Address &address);
    ~Acceptor();            // default destructor ; need to close(m_listen_fd) by myself
    Acceptor(const Acceptor &) = delete;
    Acceptor(Acceptor &&) = delete;
    Acceptor &operator=(const Acceptor &) = delete;
    Acceptor &operator=(Acceptor &&) = delete;

    void handleConnect();
    int fd() const { return m_listen_fd; }
    void Listen();

    void setNewConnectionCallback(std::function<void(int, Address)> &&cb) { m_new_connection_callback = std::move(cb); }
};

#endif