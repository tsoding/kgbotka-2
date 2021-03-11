#include <stdio.h>
#include <errno.h>
#include "./secret.h"

Secret secret_from_file(Arena *arena, String_View file_path)
{
    String_View content = {0};
    if (arena_slurp_file(arena, file_path, &content) < 0) {
        fprintf(stderr, "ERROR: could not read "SV_Fmt": %s\n",
                SV_Arg(file_path), strerror(errno));
        exit(1);
    }

    Secret result = {0};

    while (content.count > 0) {
        String_View line = sv_trim(sv_chop_by_delim(&content, '\n'));
        if (line.count > 0) {
            String_View key = sv_trim(sv_chop_by_delim(&line, '='));
            String_View value = sv_trim(line);
            if (sv_eq(key, SV("nickname"))) {
                result.nickname = value;
            } else if (sv_eq(key, SV("password"))) {
                result.password = value;
            } else if (sv_eq(key, SV("channel"))) {
                result.channel = value;
            } else {
                fprintf(stderr, "ERROR: unknown key `"SV_Fmt"`\n", SV_Arg(key));
                exit(1);
            }
        }
    }

    if (result.nickname.data == NULL) {
        fprintf(stderr, "ERROR: `nickname` was not provided\n");
        exit(1);
    }

    if (result.password.data == NULL) {
        fprintf(stderr, "ERROR: `password` was not provided\n");
        exit(1);
    }

    if (result.channel.data == NULL) {
        fprintf(stderr, "ERROR: `channel` was not provided\n");
        exit(1);
    }

    return result;
}
