#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstring>


/*
    http request format:
    GET url HTTP/1.1\r\n    
    Host: www.baidu.com\r\n
    Connection: keep-alive\r\n
*/
class HttpRequest { 
public:
    enum class Method {
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete,
        kTrace,
        kOptions,
        kConnect,
        kPatch,
        kInvalid
    };  
    enum class Version {
        kHttp10,
        kHttp11,
        kInvalid
    };

    HttpRequest(): m_method(Method::kInvalid), m_version(Version::kInvalid) {}
    ~HttpRequest() = default;

    Method method() const { return m_method; }
    Version version() const { return m_version; }
    const std::string &path() const { return m_path; }
    const std::string &query() const { return m_query; }
    const std::string &header(const std::string &field) const
    {
        std::string res;
        auto iter = m_headers.find(field);
        return iter != m_headers.end() ? iter->second : res;
    }
    
   


    void swap(HttpRequest &that)
    {
        std::swap(m_method, that.m_method);
        std::swap(m_version, that.m_version);
        m_path.swap(that.m_path);
        m_query.swap(that.m_query);
        m_headers.swap(that.m_headers);
    }

    bool parseRequestLine(const char *start,const char *end);
    void addHeader(const char *start,const char *colon,const char *end);
    bool parseRequestBody(const char *start,const char *end);
    bool parsePost(const char *start,const char *end);
private:
    bool setMethod(const char *start,const char *end)
    { 
        auto stringToMethod = [](const std::string &method) {
            if (method == "GET") return Method::kGet;
            else if (method == "POST") return Method::kPost;
            else if (method == "HEAD") return Method::kHead;
            else if (method == "PUT") return Method::kPut;
            else if (method == "DELETE") return Method::kDelete;
            else if (method == "TRACE") return Method::kTrace;
            else if (method == "OPTIONS") return Method::kOptions;
            else if (method == "CONNECT") return Method::kConnect;
            else if (method == "PATCH") return Method::kPatch;
            else return Method::kInvalid;
        };
        m_method = stringToMethod(std::string(start, end)); 
        return m_method != Method::kInvalid;
    }
    void setVersion(Version version) { m_version = version; }
    void setPath(const char *start,const char *end) { m_path.assign(start, end); }
    void setQuery(const char *start,const char *end) { m_query.assign(start, end); }
private:
    Method m_method;
    Version m_version;
    std::string m_path;
    std::string m_query;
    std::map<std::string, std::string> m_headers;
    std::map<std::string, std::string> m_post;
};

namespace std {
    template<>
    inline void swap(HttpRequest &lhs, HttpRequest &rhs)
    {
        lhs.swap(rhs);
    }
}

#endif // HTTP_REQUEST_H_