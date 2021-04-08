#include "./http.h"

bool curl_get(CURL *curl, const char *url, Region *memory, String_View *body)
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

    if (res != CURLE_OK) {
        // TODO: specific curl error should be logged in curl_get()
        return false;
    }

    return true;
}

bool curl_get_json(CURL *curl, const char *url, Region *memory, Json_Value *body)
{
    String_View raw_body = {0};
    if (!curl_get(curl, url, memory, &raw_body)) {
        return false;
    }

    Tzozen_Memory tmemory = region_to_tzozen_memory(memory);
    Json_Result result = parse_json_value(&tmemory, sv_to_tzozen_str(raw_body));
    memory->size = tmemory.size;

    if (result.is_error) {
        // TODO: specific JSON parsing error should be logged in curl_get_json()
        return false;
    }

    if (body) {
        *body = result.value;
    }

    return true;
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
