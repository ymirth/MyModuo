#include"buffer.h"
#include"httpresponse.h"

const static std::string CRLF = "\r\n";

const std::string HttpResponse::m_server_name = "Ymirth server";
void HttpResponse::appendToBuffer(Buffer * buffer) const
{
    auto code = static_cast<int>(m_status_code);
    buffer->append("HTTP/1.1 " + std::to_string(code) + " " + m_status_message + CRLF);
    if (m_close_connection)
    {
        buffer->append("Connection: close" + CRLF);
    }
    else
    {
        buffer->append("Content-length: " + std::to_string(m_body.size()) + CRLF);
        buffer->append("Connection: Keep-Alive" + CRLF);
    }

    buffer->append("Content-type: " + m_type + CRLF);
    buffer->append("Server: " + m_server_name + CRLF);
    buffer->append(CRLF);
    buffer->append(m_body);
}
