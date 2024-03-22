#include "eventloop.h"
#include "channel.h"
#include <memory>
#include "logging.h"

Channel::~Channel()
{
    if (m_loop->hasChannel(this))
    {
        this->disableAll();   // disable all events of channel in epoll
        m_loop->remove(this); // remove channel from epoll
    }
}

void Channel::update()
{
    m_loop->update(this);
}

void Channel::handleEvent() const
{
    if (m_tied)
    {
        // 在此作用块内,使用shared_ptr绑定tie，防止tie在handleEventWithGuard()中析构！！
        std::shared_ptr<void> guard = m_tie.lock();
        if (guard)
        {
            handleEventWithGuard();
        }
        // 使用shared_ptr,在if语句块结束后引用计数-1
        // 若引用计数为0,则调用guard的析构函数,解除guard与tie的绑定
    }
    else
    {
        handleEventWithGuard();
    }
}

void Channel::handleEventWithGuard() const
{
    if (m_revents & EventType::kErrorEvent)
    {
        // std::cout<<"error event fd: "<<m_fd<<std::endl;
        if (m_error_callback)
            m_error_callback();

    }
    if (m_revents & EPOLLHUP && !(m_revents & EPOLLIN))
    {
        // std::cout<<"hup event fd: "<<m_fd<<std::endl;
        if (m_close_callback)
            m_close_callback();
    }
    if (m_revents & (EventType::kReadEvent | EPOLLRDHUP))
    {
        // std::cout<<"read event fd: "<<m_fd<<std::endl;
        if (m_read_callback)
            m_read_callback();
    }
    if (m_revents & EventType::kWriteEvent)
    {
        // std::cout<<"write event fd: "<<m_fd<<std::endl;
        if (m_write_callback)
            m_write_callback();
    }
}