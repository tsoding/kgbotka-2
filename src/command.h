#ifndef COMMAND_H_
#define COMMAND_H_

#include "./sv.h"
#include "./log.h"
#include "./arena.h"

typedef struct {
    String_View name;
    String_View args;
} Command_Call;

bool command_call_parse(String_View prefix, String_View line, Command_Call *command_call);

typedef struct {
    String_View name;
    String_View response;
} Command_Def;

typedef struct {
    size_t count;
    Command_Def *command_defs;
} Commands;

bool load_commands_file(Arena *arena, Log *log, String_View file_path, Commands *
commands);

bool commands_find_def(const Commands *commands, String_View name, Command_Def *def);

#endif // COMMAND_H_
