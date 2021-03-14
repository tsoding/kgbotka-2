#ifndef REGION_H_
#define REGION_H_

#include <stdlib.h>

#include "./sv.h"

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


#endif // REGION_H_
