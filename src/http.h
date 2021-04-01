#ifndef HTTP_H_
#define HTTP_H_

#include <curl/curl.h>

#include "./region.h"

CURLcode curl_get(CURL *curl, const char *url, Region *memory, String_View *body);

String_View url_encode(Region *memory, String_View sv);

#endif // HTTP_H_
