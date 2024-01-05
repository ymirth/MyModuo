# Webserver

A web server based on Muduo.

## Description

This web server is built on the Muduo framework, incorporating various features and design patterns for efficient and scalable performance.

## Features

### Epoll IO Multiplexing

- Utilizes epoll to monitor channel events.

### Master-Slave Reactor Pattern (One Loop Per Thread)

#### Eventloop

- Reactor implemented by the eventloop.
- Handles channel events:
  - Acceptor event: Accepts new connections.
  - Tcpconnection events: Read/Write/Close.
  - Timer events: Timeout.
  - Async events: Wakes up epoll_wait.
- Async events:
  - Ensures the safety of each eventloop.
  - Uses mutex to store and retrieve events in the queue.
  - Queues functions to be executed in the specific eventloop.
  - Utilizes eventfd to wake up epoll_wait.

#### Threadpool

- Creates a slave-reactor (eventloop) in each thread.
- Each eventloop stores the thread id.
- Uses round-robin to choose the eventloop.

#### Master-Slave TcpServer

- Master reactor: Base event_loop (created in the main thread).
  - Acceptor: Accepts new connections and distributes to slave reactors.
- Slave reactor: Event_loop (created in the threadpool).
  - Tcpconnection: Read/Write/Close.
  - Timer: Timeout.
  - Async: Executes functions queued in other threads.

### Timer based on RBTree: Close Timeout Connections

- RBTreeTimer:
  - Uses an RBTree to store timers.
  - Uses timerfd to monitor the earliest timeout event.
  - Resets timerfd when the earliest timer times out or changes.

### Http State Machine

- Utilizes a state machine to parse HTTP requests.
  - Uses enum class to represent states:
    - REQUEST_LINE
    - HEADERS
    - BODY
    - FINISH
  - Uses a switch to change states.

### Buffer: Scattered Read and Gathered Write

- Uses vector<char> to store data.
- Utilizes readv for scattered read, then increases the buffer size.
- Uses write for gathered write.

## Resource Ownership

### FD Ownership: RAII

- acceptor: listen_fd, idle_fd
- tcpconnection: conn_fd
- epoller: epoll_fd
- eventloop: wakeup_fd
- timer: timer_fd

### Channel Ownership: Unique_ptr

- acceptor: accept_channel(listen_fd, base_loop)
- tcpconnection: conn_channel(conn_fd, conn_loop)
- eventloop: wakeup_channel(wakeup_fd, this_loop)
- timer: timer_channel(timer_fd, m_loop)

## TODO

- [ ] Add logging.
- [ ] Implement database support.
- [ ] Add configuration options.
- [ ] Implement comprehensive testing.
