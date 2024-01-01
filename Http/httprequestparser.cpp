#include "httprequestparser.h"

HttpRequestParseState HttpRequestParser::parseContent(Buffer *buffer)
{
    while (buffer->readableBytes() > 0)
    {
        const char *crlf = buffer->findCRLF();
        if (crlf == nullptr)
            return m_state;
        const char *lineStart = buffer->curReadPtr();
        buffer->retrieveUntilIndex(crlf + 2); // 2: CRLF
        switch (m_state)
        {
        case HttpRequestParseState::kParseRequestLine:
        { // parseRequestLine(const char *start, const char *end)
            if (m_request.parseRequestLine(lineStart, crlf))
            {
                m_state = HttpRequestParseState::kParseHeader;
            }
            else
            {
                m_state = HttpRequestParseState::kParseErrno;
                return m_state;
            }
            break;
        }
        case HttpRequestParseState::kParseHeader:
        { // parseHeader(const char *start, const char *end)
            auto line = std::string(lineStart, crlf);
            if (line.size() == 0)
            {
                m_state = HttpRequestParseState::kParseBody;

                m_state = HttpRequestParseState::kParseGotCompleteRequest;
                return m_state;
            }
            else
            {
                auto colon = std::find(lineStart, crlf, ':');
                if (colon != crlf)
                {
                    m_request.addHeader(lineStart, colon, crlf);
                }
                else
                {
                    m_state = HttpRequestParseState::kParseErrno;
                    return m_state;
                }
            }
            break;
        }
        case HttpRequestParseState::kParseBody:
        {
            m_state = HttpRequestParseState::kParseGotCompleteRequest;
            return m_state;
            if (m_request.parseRequestBody(lineStart, crlf))
            {
                m_state = HttpRequestParseState::kParseGotCompleteRequest;
                return m_state;
            }
            else
            {
                m_state = HttpRequestParseState::kParseErrno;
                return m_state;
            }
            break;
        }
        default:
        {
            m_state = HttpRequestParseState::kParseErrno;
            return m_state;
        }
        }
    }
    return m_state;
}
