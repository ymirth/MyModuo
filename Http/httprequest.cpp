#include "httprequest.h"

#include <regex>

/*
    http request format:
    GET url HTTP/1.1\r\n
    Host: www.baidu.com\r\n
    Connection: keep-alive\r\n
*/
bool HttpRequest::parseRequestLine(const char *start, const char *end)
{
    const char *space = std::find(start, end, ' ');
    if (space == end)
        return false;
    setMethod(start, space);
    if (m_method == Method::kInvalid)
        return false;

    // set path/query/verison
    const char *path_start = space + 1;
    space = std::find(path_start, end, ' ');
    if (space == end)
        return false;
    setPath(path_start, space);

    const char *query_start = std::find(path_start, space, '?');
    if (query_start != space)
    {
        setPath(path_start, query_start);
        setQuery(query_start + 1, space);
    }
    else
    {
        setPath(path_start, space);
    }

    const char *version_start = space + 1;
    if (end - version_start != 8)
        return false;
    if (strncasecmp(version_start, "HTTP/1.1", 8) == 0)
    {
        setVersion(Version::kHttp11);
    }
    else if (strncasecmp(version_start, "HTTP/1.0", 8) == 0)
    {
        setVersion(Version::kHttp10);
    }
    else
    {
        return false;
    }
    return true;
}

void HttpRequest::addHeader(const char *start, const char *colon, const char *end)
{   // colon is the first char of ": "
        std::string field(start, colon);        // [start, colon)
        ++colon;                                // skip ": "
        while (colon < end && isspace(*colon))  // skip " "
        {
            ++colon;
        }
        std::string value(colon, end);          // [colon, end)
        while (!value.empty() && isspace(value[value.size() - 1]))
        {
            value.resize(value.size() - 1);     // skip " " in the end
        }
        m_headers[field] = value;
}

bool HttpRequest::parseRequestBody(const char *start, const char *end)
{
    if (m_method == Method::kPost)
    {
        return parsePost(start, end);
    }
    return true;
}

bool HttpRequest::parsePost(const char *start, const char *end)
{
    std::string body(start, end);
    std::regex pattern("([^=&]*)=([^=&]*)");
    std::smatch res;
    while (std::regex_search(body, res, pattern))
    {
        m_post[res[1].str()] = res[2].str();
        body = res.suffix().str();
    }
    return true;
}


