#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdlib.h>

#include "./sv.h"

typedef struct {
    size_t size;
    size_t capacity;
    char *data;
} Buffer;

void buffer_resize(Buffer *buffer, size_t new_capacity);
void buffer_write(Buffer *buffer, const char *data, size_t size);
void buffer_free(Buffer *buffer);
String_View sv_from_buffer(Buffer buffer);

#endif // BUFFER_H_
