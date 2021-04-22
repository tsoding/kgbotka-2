#include "./discord.h"

const char *discord_opcode_as_cstr(Discord_Opcode opcode)
{
    switch (opcode) {
    case DISCORD_OPCODE_DISPATCH:
        return "DISCORD_OPCODE_DISPATCH";
    case DISCORD_OPCODE_HEARTBEAT:
        return "DISCORD_OPCODE_HEARTBEAT";
    case DISCORD_OPCODE_IDENTIFY:
        return "DISCORD_OPCODE_IDENTIFY";
    case DISCORD_OPCODE_PRESENCE:
        return "DISCORD_OPCODE_PRESENCE";
    case DISCORD_OPCODE_VOICE:
        return "DISCORD_OPCODE_VOICE";
    case DISCORD_OPCODE_RESUME:
        return "DISCORD_OPCODE_RESUME";
    case DISCORD_OPCODE_RECONNECT:
        return "DISCORD_OPCODE_RECONNECT";
    case DISCORD_OPCODE_REQUEST:
        return "DISCORD_OPCODE_REQUEST";
    case DISCORD_OPCODE_INVALID:
        return "DISCORD_OPCODE_INVALID";
    case DISCORD_OPCODE_HELLO:
        return "DISCORD_OPCODE_HELLO";
    case DISCORD_OPCODE_HEARTBEAT_ACK:
        return "DISCORD_OPCODE_HEARTBEAT_ACK";
    default:
        return "(Unknown)";
    }
}

bool extract_discord_gateway_url(Json_Value discord_gateway_response,
                                 String_View *gateway_url)
{
    if (discord_gateway_response.type != JSON_OBJECT) return false;

    Json_Value url = json_object_value_by_key(discord_gateway_response.object, TSTR("url"));

    if (url.type != JSON_STRING) return false;

    if (gateway_url) {
        gateway_url->count = url.string.len;
        gateway_url->data = url.string.data;
    }

    return true;
}

bool discord_deserialize_payload(Json_Value json_payload, Discord_Payload *payload)
{
    if (json_payload.type != JSON_OBJECT) return false;

    Json_Value json_op = json_object_value_by_key(json_payload.object, TSTR("op"));
    if (json_op.type != JSON_NUMBER) return false;
    payload->opcode = json_number_to_integer(json_op.number);

    switch (payload->opcode) {
    case DISCORD_OPCODE_HELLO: {
        Json_Value json_d = json_object_value_by_key(json_payload.object, TSTR("d"));
        if (json_d.type != JSON_OBJECT) return false;

        Json_Value json_heartbeat_interval =
            json_object_value_by_key(json_d.object, TSTR("heartbeat_interval"));
        if (json_heartbeat_interval.type != JSON_NUMBER) return false;
        payload->hello.heartbeat_interval =
            json_number_to_integer(json_heartbeat_interval.number);
    } break;

    default: {}
    }

    return true;
}
