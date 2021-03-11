#ifndef SECRET_H_
#define SECRET_H_

#include "./sv.h"
#include "./arena.h"

typedef struct {
    String_View nickname;
    String_View password;
    String_View channel;
} Secret;

Secret secret_from_file(Arena *arena, String_View file_path);

#endif // SECRET_H_
