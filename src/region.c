#include <string.h>
#include <stdarg.h>
#include "./region.h"

Region *region_new(size_t capacity)
{
    Region *result = malloc(sizeof(Region) + capacity);
    if (result == NULL) {
        return NULL;
    }

    memset(result, 0, sizeof(Region) + capacity);

    result->capacity = capacity;

    return result;
}

void *region_alloc(Region *region, size_t size)
{
    if (region->size + size >= region->capacity) {
        return NULL;
    }

    void *result = region->buffer + region->size;
    memset(result, 0, size);
    region->size += size;
    return result;
}

void *region_write(Region *region, char *data, size_t size)
{
    void *result = region_alloc(region, size);
    if (result == NULL) {
        return result;
    }

    memcpy(result, data, size);
    return result;
}

void region_clean(Region *region)
{
    region->size = 0;
}

void region_free(Region *region)
{
    free(region);
}

String_View region_sv_concat(Region *region, ...)
{
    size_t len = 0;

    va_list args;
    va_start(args, region);
    String_View sv = va_arg(args, String_View);
    while (sv.data != NULL) {
        len += sv.count;
        sv = va_arg(args, String_View);
    }
    va_end(args);

    char *buffer = region_alloc(region, len);
    if (buffer == NULL) {
        return SV_NULL;
    }
    len = 0;

    va_start(args, region);
    sv = va_arg(args, String_View);
    while (sv.data != NULL) {
        memcpy(buffer + len, sv.data, sv.count);
        len += sv.count;
        sv = va_arg(args, String_View);
    }
    va_end(args);

    return (String_View) {
        .count = len,
        .data = buffer
    };
}

size_t write_to_region(char *data, size_t size, size_t nmemb, Region *region)
{
    void *dest = region_alloc(region, size * nmemb);
    if (dest == NULL) {
        return 0;
    }
    memcpy(dest, data, size * nmemb);
    return nmemb;
}

Tzozen_Memory region_to_tzozen_memory(Region *region)
{
    Tzozen_Memory memory = {0};
    memory.capacity = region->capacity;
    memory.size = region->size;
    memory.buffer = (uint8_t *) region->buffer;
    return memory;
}
