#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include"httpstate.h"

class Buffer;

class HttpResponse
{
public:
    explicit HttpResponse(bool close)
        : m_close_connection(close), m_type("text/html"), m_status_code(HttpStatusCode::k200OK){}
    ~HttpResponse() = default;

    void setStatusCode(HttpStatusCode code) { m_status_code = code; }
    void setStatusMessage(const std::string message) { m_status_message = message; }
    void setCloseConnection(bool on) { m_close_connection = on; }
    void setContentType(const std::string &type) { m_type = type; }
    void setBody(const std::string &body) { m_body = body; }

    void appendToBuffer(Buffer *output) const;
    bool closeConnection() const { return m_close_connection; }
private:
    HttpStatusCode m_status_code;
    std::string m_status_message; 
    
    std::string m_body;
    std::string m_type;
    static const std::string m_server_name;

    bool m_close_connection;
};

#endif // HTTP_RESPONSE_H_