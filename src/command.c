#include <errno.h>

#include "./command.h"

bool command_call_parse(String_View prefix, String_View line, Command_Call *command_call)
{
    line = sv_trim(line);

    if (!sv_starts_with(line, prefix)) {
        return false;
    }

    sv_chop_left(&line, prefix.count);
    line = sv_trim(line);

    String_View name = sv_chop_by_delim(&line, ' ');

    if (name.count == 0) {
        return false;
    }

    if (command_call) {
        command_call->name = name;
        command_call->args = line;
    }

    return true;
}

bool load_commands_file(Arena *arena, Log *log, String_View file_path, Commands *commands)
{
    log_info(log, "Reloading commands from `"SV_Fmt"`", SV_Arg(file_path));

    String_View content = {0};
    if (arena_slurp_file(arena, file_path, &content) < 0) {
        log_warning(log, "Could not read file `"SV_Fmt"`: %s",
                    SV_Arg(file_path), strerror(errno));
        return false;
    }

    size_t capacity = 0;
    {
        String_View iter = content;
        while (iter.count > 0) {
            String_View line = sv_trim(sv_chop_by_delim(&iter, '\n'));
            if (line.count > 0) {
                capacity += 1;
            }
        }
    }

    Command_Def *command_defs = arena_alloc(arena, sizeof(Command_Def) * capacity);
    if (command_defs == NULL) {
        log_warning(log, "Could not allocate memory for command definitions: %s\n",
                    strerror(errno));
        return false;
    }

    {
        String_View iter = content;
        size_t count = 0;
        while (iter.count > 0) {
            String_View line = sv_trim(sv_chop_by_delim(&iter, '\n'));
            if (line.count > 0) {
                if (count < capacity) {
                    command_defs[count].name = sv_trim(sv_chop_by_delim(&line, '='));
                    command_defs[count].response = sv_trim(line);
                    count += 1;
                } else {
                    log_error(log, "Trying to add more command definitions than expected");
                    exit(1);
                }
            }
        }
    }

    commands->count = capacity;
    commands->command_defs = command_defs;

    log_info(log, "Successfully reloaded commands");

    return true;
}

bool commands_find_def(const Commands *commands, String_View name, Command_Def *def)
{
    for (size_t i = 0; i < commands->count; ++i) {
        if (sv_eq(commands->command_defs[i].name, name)) {
            if (def) {
                *def = commands->command_defs[i];
            }
            return true;
        }
    }
    return false;
}
