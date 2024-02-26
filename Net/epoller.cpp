#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <vector>
#include <assert.h>
#include <iostream>

#include "channel.h"
#include "epoller.h"
#include "logging.h"

Epoller::Epoller(int max_event_num) : 
    m_epoll_fd(::epoll_create1(EPOLL_CLOEXEC)), 
    m_events(max_event_num)
{
    if (m_epoll_fd < 0)
    {
        perror("epoll_create");
        exit(1);
    }
}
Epoller::~Epoller()
{
    ::close(m_epoll_fd);
}
Epoller::Epoller(Epoller &&epoller) :
    m_epoll_fd(epoller.m_epoll_fd), m_events(std::move(epoller.m_events))
{
    m_epoll_fd = -1;
}
Epoller &Epoller::operator=(Epoller &&epoller)
{
    if (this != &epoller)
    {
        m_epoll_fd = epoller.m_epoll_fd;
        m_events = std::move(epoller.m_events);
        epoller.m_epoll_fd = -1;
    }
    return *this;
}

bool Epoller::hasChannel(Channel *channel) const
{
    auto it = m_channels.find(channel->fd());
    return it != m_channels.end() && it->second == channel;
}

void Epoller::updateChannel(Channel *channel)
{
    ChannelIndex index = channel->index();
    if(index == ChannelIndex::kNew || index == ChannelIndex::kDeleted) 
    {
        int fd = channel->fd();
        if(index == ChannelIndex::kNew)
        {
            m_channels[fd] = channel;
        }
        else // index == ChannelIndex::kDeleted
        {// ensure fd is in m_channels to call update(op, channel)
            assert(m_channels.find(fd) != m_channels.end()); // fd must be in m_channels
            assert(m_channels[fd] == channel);               // channel must be in m_channels
        }
        channel->setIndex(ChannelIndex::kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // index == ChannelIndex::kAdded
    {
        int fd = channel->fd();
        assert(m_channels.find(fd) != m_channels.end());
        assert(m_channels[fd] == channel);
        
        if(channel->isRegistered())  // events != 0
        {
            update(EPOLL_CTL_MOD, channel);
        }
        else
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(ChannelIndex::kDeleted);
        }
    }

}

void Epoller::removeChannel(Channel *channel)
{// channel == kDeleted or kAdded
    int fd = channel->fd();
    auto index = channel->index();
    
    assert(m_channels.find(fd) != m_channels.end());
    assert(m_channels[fd] == channel);
    
    assert(index == ChannelIndex::kAdded || index == ChannelIndex::kDeleted);
    if(index == ChannelIndex::kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
        channel->setIndex(ChannelIndex::kDeleted);
    }
   
    m_channels.erase(fd);
}

void Epoller::update(int op, Channel *channel)
{
    struct epoll_event event;
    event.events = channel->events();
    event.data.ptr = channel;         // void * <- Channel *
    if (epoll_ctl(m_epoll_fd, op, channel->fd(), &event) < 0)
    {
        LOG_ERROR << "Epoller::UpdataChannel epoll_ctl failed\n"; 
    }
}

void Epoller::fillActiveChannels(int num_events, std::vector<Channel *> &active_channels) const
{
    for(int i=0;i<num_events;i++)
    {
        Channel *channel = static_cast<Channel *>(m_events[i].data.ptr);
        channel->setRevents(m_events[i].events);
        active_channels.push_back(channel);
    }
}

void Epoller::poll(std::vector<Channel *> &active_channels)
{
    int num_events = this->wait();
    if(num_events < 0)
    {
        int error_code = errno;
        // 根据错误类型进行特定的处理
        if (error_code == EBADF) {
            fprintf(stderr, "Invalid file descriptor.\n");
        } else if (error_code == EFAULT) {
            fprintf(stderr, "Bad address.\n");
        } else if (error_code == EINTR) {
            fprintf(stderr, "Interrupted system call.\n");
        }
        return;
    }
    fillActiveChannels(num_events, active_channels);
}