#ifndef CHANNEL_H
#define CHANNEL_H

#include<functional>
#include<memory>
#include<sys/epoll.h>
#include<unistd.h>
#include<errno.h>
#include<assert.h>

enum class ChannelIndex { kNew , kAdded , kDeleted };

class EventLoop;

class Channel{
private:
    void update();
public:
    Channel(EventLoop* loop, int n_fd) : m_fd(n_fd), m_loop(loop), m_events(0), m_revents(0), m_index(ChannelIndex::kNew) {}
    ~Channel() {}
    
    int fd() const { return m_fd; }
    uint32_t events() const { return m_events; }
    void setEvents(uint32_t evt) { m_events = evt; }
    void setRevents(uint32_t revt) { m_revents = revt; }

    ChannelIndex index() const { return m_index; }                     // kNew, kAdded, kDeleted
    void setIndex(ChannelIndex idx) { m_index = idx; }       // kNew, kAdded, kDeleted
    bool isRegistered() const { return m_events != 0; }       // no events(EPOLLIN or  EPOLLOUT) registered

    void setReadCallback(std::function<void()> &&cb) { m_read_callback = std::move(cb); }
    void setWriteCallback(std::function<void()> &&cb) { m_write_callback = std::move(cb); }
    void setErrorCallback(std::function<void()> &&cb) { m_error_callback = std::move(cb); }
    void setCloseCallback(std::function<void()> &&cb) { m_close_callback = std::move(cb); }
private:
    bool m_tied;                                              // whether tie to a specific object
    std::weak_ptr<void> m_tie;                                // weak_ptr to tie object
    ChannelIndex m_index;                                       // kNew, kAdded, kDeleted

    EventLoop *m_loop;                                        // Channel belongs to which EventLoop
    const int m_fd;                                           // Channel's fd
    uint32_t m_events;                                        // events type registered in epoll
    uint32_t m_revents;                                       // events type that epoll returns
    std::function<void()> m_read_callback;
    std::function<void()> m_write_callback;
    std::function<void()> m_error_callback;
    std::function<void()> m_close_callback;  // used in function: handleClose()

public:
    void Tie(const std::shared_ptr<void> &obj)
    { 
        m_tied = true;
        m_tie = obj;
    }

    void handleEvent() const;
    void handleEventWithGuard() const;

    void enableReading() { m_events |= (EPOLLIN | EPOLLPRI); update(); }
    void enableWriting() { m_events |= EPOLLOUT; update(); }
    void disableReading() { m_events &= ~EPOLLIN; update(); }
    void disableWriting() { m_events &= ~EPOLLOUT; update(); }
    void disableAll() { m_events = 0; update(); }
    bool isReading() const { return (m_events & (EPOLLIN | EPOLLPRI)); }   // whether EPOLLIN is registered
    bool isWriting() const { return (m_events & EPOLLOUT); }               // whether EPOLLOUT is registered
};

#endif