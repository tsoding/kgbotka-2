#include "./http.h"

CURLcode curl_get(CURL *curl, const char *url, Region *memory, String_View *body)
{
    size_t begin_size = memory->size;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_region);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, memory);

    CURLcode res = curl_easy_perform(curl);
    if (body) {
        body->count = memory->size - begin_size;
        body->data = memory->buffer + begin_size;
    }
    return res;
}

static char hex_char(char x)
{
    if (0 <= x && x < 10) return x + '0';
    if (10 <= x && x < 16) return x - 10 + 'A';
    return ')';
}

String_View url_encode(Region *memory, String_View sv)
{
    const size_t WIDTH = 3;
    char *result = region_alloc(memory, sv.count * WIDTH);
    if (result == NULL) {
        return SV_NULL;
    }

    for (size_t i = 0; i < sv.count; ++i) {
        result[i * WIDTH + 0] = '%';
        result[i * WIDTH + 1] = hex_char(((sv.data[i]) >> 4) & 0xf);
        result[i * WIDTH + 2] = hex_char((sv.data[i]) & 0xf);
    }

    return (String_View) {
        .count = sv.count * WIDTH,
        .data = result,
    };
}
