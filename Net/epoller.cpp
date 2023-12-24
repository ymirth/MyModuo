#include "channel.h"
#include "epoller.h"

#include <vector>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>

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

bool Epoller::setNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0)
    {
        perror("fcntl");
        return false;
    }
    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) < 0)
    {
        perror("fcntl");
        return false;
    }
    return true;
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
        else
        {
            assert(m_channels.find(fd) != m_channels.end());
            assert(m_channels[fd] == channel);
        }
        channel->setIndex(ChannelIndex::kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        assert(m_channels.find(fd) != m_channels.end());
        assert(m_channels[fd] == channel);
        assert(index == ChannelIndex::kAdded);
        if(channel->EventRegistered())
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
{
    int fd = channel->fd();
    ChannelIndex index = channel->index();

    assert(m_channels.find(fd) != m_channels.end());
    assert(m_channels[fd] == channel);
    assert(channel->isRegistered());
    assert(channel->index() == ChannelIndex::kAdded);
    
    if(index == ChannelIndex::kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    m_channels.erase(fd);  // remove from map， must before set_index

    channel->setIndex(ChannelIndex::kDeleted);
    
}

void Epoller::update(int op, Channel *channel)
{
    struct epoll_event event;
    event.events = channel->events();
    event.data.ptr = channel;         // void * <- Channel *
    if (epoll_ctl(m_epoll_fd, op, channel->fd(), &event) < 0)
    {
        perror("epoll_ctl");
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
        perror("epoll_wait");
        return;
    }
    fillActiveChannels(num_events, active_channels);
}