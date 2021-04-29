#ifndef DISCORD_H_
#define DISCORD_H_

#include "./thirdparty/tzozen.h"
#include "./thirdparty/jim.h"
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
    uint64_t heartbeat_interval;
} Discord_Hello;

// https://discord.com/developers/docs/topics/gateway#gateway-intents
typedef enum {
    GATEWAY_INTENT_GUILDS                   = (1 << 0),
    GATEWAY_INTENT_GUILD_MEMBERS            = (1 << 1),
    GATEWAY_INTENT_GUILD_BANS               = (1 << 2),
    GATEWAY_INTENT_GUILD_EMOJIS             = (1 << 3),
    GATEWAY_INTENT_GUILD_INTEGRATIONS       = (1 << 4),
    GATEWAY_INTENT_GUILD_WEBHOOKS           = (1 << 5),
    GATEWAY_INTENT_GUILD_INVITES            = (1 << 6),
    GATEWAY_INTENT_GUILD_VOICE_STATES       = (1 << 7),
    GATEWAY_INTENT_GUILD_PRESENCES          = (1 << 8),
    GATEWAY_INTENT_GUILD_MESSAGES           = (1 << 9),
    GATEWAY_INTENT_GUILD_MESSAGE_REACTIONS  = (1 << 10),
    GATEWAY_INTENT_GUILD_MESSAGE_TYPING     = (1 << 11),
    GATEWAY_INTENT_DIRECT_MESSAGES          = (1 << 12),
    GATEWAY_INTENT_DIRECT_MESSAGE_REACTIONS = (1 << 13),
    GATEWAY_INTENT_DIRECT_MESSAGE_TYPING    = (1 << 14),
} Discord_Gateway_Intents;

// https://discord.com/developers/docs/topics/gateway#identify-identify-connection-properties
typedef struct {
    String_View os;
    String_View browser;
    String_View device;
} Discord_Conn_Prop;

// https://discord.com/developers/docs/topics/gateway#identify
typedef struct {
    Discord_Opcode opcode;
    String_View token;
    Discord_Conn_Prop properties;
    Discord_Gateway_Intents intents;
} Discord_Identify;

typedef union {
    Discord_Opcode opcode;
    Discord_Hello hello;
    Discord_Identify identify;
} Discord_Payload;

void serialize_discord_conn_prop(Jim *jim, Discord_Conn_Prop properties);
void serialize_discord_payload(Jim *jim, Discord_Payload payload);
void serialize_discord_identify(Jim *jim, Discord_Identify identify);
void serialize_discord_hello(Jim *jim, Discord_Hello hello);

bool discord_deserialize_payload(Json_Value json_payload, Discord_Payload *payload);

bool extract_discord_gateway_url(Json_Value discord_gateway_response,
                                 String_View *gateway_url);

#endif // DISCORD_H_
