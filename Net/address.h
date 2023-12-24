#ifndef ARRESS_H
#define ARRESS_H

#include <string>
#include <netinet/in.h>

class Address{
private:
    const char* m_ip;
    int m_port;
public:
    Address(const char* ip, int port) : m_ip(ip), m_port(port) {}

    const char* ip() const { return m_ip; }
    int port() const { return m_port; }
};

#endif