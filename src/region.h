#ifndef REGION_H_
#define REGION_H_

#include <stdlib.h>

#include "./sv.h"
#include "./thirdparty/tzozen.h"

typedef struct {
    size_t capacity;
    size_t size;
    char buffer[];
} Region;

Region *region_new(size_t capacity);
void *region_alloc(Region *region, size_t size);
void region_clean(Region *region);
void region_free(Region *region);

String_View region_sv_concat(Region *region, ...);

#define SV_CONCAT(region, ...)                   \
    region_sv_concat(region, __VA_ARGS__, SV_NULL)

char *region_sv_to_cstr(Region *region, String_View sv);

size_t write_to_region(const char *data, size_t size, size_t nmemb, Region *region);

Tzozen_Memory region_to_tzozen_memory(Region *region);

#endif // REGION_H_
