#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdlib.h>

typedef struct {
    size_t size;
    size_t capacity;
    char *data;
} Buffer;

void buffer_resize(Buffer *buffer, size_t new_capacity);
void buffer_write(Buffer *buffer, const char *data, size_t size);
void buffer_free(Buffer *buffer);

#endif // BUFFER_H_
