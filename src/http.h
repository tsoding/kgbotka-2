#ifndef HTTP_H_
#define HTTP_H_

#include <curl/curl.h>

#include "./region.h"
#include "./thirdparty/tzozen.h"

bool curl_get(CURL *curl, const char *url, Region *memory, String_View *body);
bool curl_get_json(CURL *curl, const char *url, Region *memory, Json_Value *body);

String_View url_encode(Region *memory, String_View sv);

#endif // HTTP_H_
