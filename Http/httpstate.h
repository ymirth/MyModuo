#ifndef HTTP_STATE_H
#define HTTP_STATE_H

enum class HttpRequestParseState {
  kParseRequestLine,
  kParseHeader,
  kParseBody,
  kParseGotCompleteRequest,
  kParseErrno,
};

enum class HttpStatusCode {
  k100Continue = 100,
  k200OK = 200,
  k400BadRequest = 400,
  k403Forbidden = 403,
  k404NotFound = 404,
  k500InternalServerErrno = 500
};

#endif // HTTP_STATE_H_