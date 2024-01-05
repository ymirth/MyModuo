#ifndef EPOLLER_H
#define EPOLLER_H

#include<vector>
#include<map>
#include<sys/epoll.h>


/*
    Epoller与Channel的关系：
    1. Epoller负责管理Channel，Epoller中有一个ChannelMap，ChannelMap中存放着fd到Channel的映射
    2. Poll: 调用epoll_wait，将返回的事件填充到m_events中, 并将活跃的Channel填充到active_channels中
*/
class Channel;


class Epoller{
private:
    using ChannelMap = std::map<int, Channel *>; // fd -> Channel *
    int m_epoll_fd;                              // epoll_create返回的文件描述符
    std::vector<struct epoll_event> m_events;    // epoll_wait 传入传出参数 记录活跃的事件
    ChannelMap m_channels;                       // 记录fd到Channel的映射
public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();
    #pragma region delete copy and move
    Epoller(const Epoller &) = delete;
    Epoller(Epoller &&epoller);
    Epoller &operator=(const Epoller &) = delete;
    Epoller &operator=(Epoller &&epoller);
    #pragma endregion delete copy and move
   
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    void update(int op, Channel *channel);

    void poll(std::vector<Channel *> &active_channels);

    void fillActiveChannels(int num_events, std::vector<Channel *> &active_channels) const;

    bool hasChannel(Channel *channel) const;

    int wait(int timeout_ms = -1)
    {  // -1 means wait forever
        return epoll_wait(m_epoll_fd, &m_events[0], static_cast<int>(m_events.size()), timeout_ms);
    }
};

#endif