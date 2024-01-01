#ifndef HTTP_HTTPREQUESTPARSER_H
#define HTTP_HTTPREQUESTPARSER_H

#include"httprequest.h"
#include"httpstate.h"
#include"buffer.h"



// description: 
class HttpRequestParser {
public:
    HttpRequestParser(): m_state(HttpRequestParseState::kParseRequestLine) {}
    ~HttpRequestParser() = default;

    HttpRequestParseState parseContent(Buffer* buffer);
    bool gotAll() const { return m_state == HttpRequestParseState::kParseGotCompleteRequest; }

    HttpRequest &getRequest() { return m_request; }
    const HttpRequest &getRequest() const { return m_request; }
    void reset()
    {
        HttpRequest tmp;
        std::swap(tmp, m_request);  // use template specialization std::swap
        m_state = HttpRequestParseState::kParseRequestLine;
    }
private:
    HttpRequest m_request;
    HttpRequestParseState m_state;
};

#endif // HTTP_CONTENT_H_