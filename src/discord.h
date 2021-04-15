#ifndef DISCORD_H_
#define DISCORD_H_

#include "./thirdparty/tzozen.h"
#include "./sv.h"

typedef enum {
    DISCORD_OPCODE_DISPATCH = 0,
    DISCORD_OPCODE_HEARTBEAT = 1,
    DISCORD_OPCODE_IDENTIFY = 2,
    DISCORD_OPCODE_PRESENCE = 3,
    DISCORD_OPCODE_VOICE = 4,
    DISCORD_OPCODE_RESUME = 6,
    DISCORD_OPCODE_RECONNECT = 7,
    DISCORD_OPCODE_REQUEST = 8,
    DISCORD_OPCODE_INVALID = 9,
    DISCORD_OPCODE_HELLO = 10,
    DISCORD_OPCODE_HEARTBEAT_ACK = 11,
} Discord_Opcode;

const char *discord_opcode_as_cstr(Discord_Opcode opcode);

typedef struct {
    Discord_Opcode opcode;
} Discord_Payload;

bool discord_deserialize_payload(Json_Value json_payload, Discord_Payload *payload);

bool extract_discord_gateway_url(Json_Value discord_gateway_response,
                                 String_View *gateway_url);

#endif // DISCORD_H_
