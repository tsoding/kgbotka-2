#include <string.h>
#include "./buffer.h"

void buffer_resize(Buffer *buffer, size_t new_capacity)
{
    buffer->capacity = new_capacity;
    buffer->data = realloc(buffer->data, buffer->capacity);
}

void buffer_write(Buffer *buffer, const char *data, size_t size)
{
    if (buffer->size + size > buffer->capacity) {
        buffer_resize(buffer, buffer->capacity * 2 + size);
    }

    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
}

void buffer_free(Buffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

