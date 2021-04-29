#include "./discord.h"
#include "./log.h"
#include "./json.h"
#include "./thirdparty/cws.h"

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

bool deserialize_discord_payload(Json_Value json_payload, Discord_Payload *payload)
{
    if (json_payload.type != JSON_OBJECT) return false;

    // op
    {
        Json_Value json_op = json_object_value_by_key(json_payload.object, TSTR("op"));
        if (json_op.type != JSON_NUMBER) return false;
        payload->op = json_number_to_integer(json_op.number);
    }

    // d
    {
        switch (payload->op) {
        case DISCORD_OPCODE_HELLO: {
            Json_Value json_d = json_object_value_by_key(json_payload.object, TSTR("d"));
            if (json_d.type != JSON_OBJECT) return false;

            Json_Value json_heartbeat_interval =
                json_object_value_by_key(json_d.object, TSTR("heartbeat_interval"));
            if (json_heartbeat_interval.type != JSON_NUMBER) return false;
            payload->d.hello.heartbeat_interval =
                json_number_to_integer(json_heartbeat_interval.number);
        }
        break;

        default:
        {}
        }
    }

    // t
    {
        Json_Value json_t = json_object_value_by_key(json_payload.object, TSTR("t"));
        if (json_t.type == JSON_STRING) {
            payload->t.data = json_t.string.data;
            payload->t.count = json_t.string.len;
        }
    }

    return true;
}

void serialize_discord_conn_prop(Jim *jim, Discord_Conn_Prop properties)
{
    jim_object_begin(jim);
    {
        jim_member_key(jim, "$os");
        jim_string_sized(jim, properties.os.data, properties.os.count);

        jim_member_key(jim, "$browser");
        jim_string_sized(jim, properties.browser.data, properties.browser.count);

        jim_member_key(jim, "$device");
        jim_string_sized(jim, properties.device.data, properties.device.count);
    }
    jim_object_end(jim);
}

void serialize_discord_identify(Jim *jim, Discord_Identify identify)
{
    jim_object_begin(jim);
    {
        jim_member_key(jim, "token");
        jim_string_sized(jim, identify.token.data, identify.token.count);

        jim_member_key(jim, "properties");
        serialize_discord_conn_prop(jim, identify.properties);

        jim_member_key(jim, "intents");
        jim_integer(jim, identify.intents);
    }
    jim_object_end(jim);
}

void serialize_discord_hello(Jim *jim, Discord_Hello hello)
{
    jim_object_begin(jim);
    {
        jim_member_key(jim, "heartbeat_interval");
        jim_integer(jim, hello.heartbeat_interval);
    }
    jim_object_end(jim);
}

void serialize_discord_payload(Jim *jim, Discord_Payload payload)
{
    jim_object_begin(jim);
    {
        jim_member_key(jim, "op");
        jim_integer(jim, payload.op);

        jim_member_key(jim, "d");
        switch (payload.op) {
        case DISCORD_OPCODE_HELLO:
            serialize_discord_hello(jim, payload.d.hello);
            break;

        case DISCORD_OPCODE_IDENTIFY:
            serialize_discord_identify(jim, payload.d.identify);
            break;

        default:
            assert(false && "unreachable");
        }

        jim_member_key(jim, "t");
        jim_string_sized(jim, payload.t.data, payload.t.count);
    }
    jim_object_end(jim);
}

static String_View cws_message_chunk_to_sv(Cws_Message_Chunk chunk)
{
    return (String_View) {
        .data = (const char *) chunk.payload,
        .count = chunk.payload_len,
    };
}

bool receive_discord_payload_from_websocket(Cws *cws, Log *log, Region *memory, Discord_Payload *payload)
{
    Cws_Message message = {0};
    if (cws_read_message(cws, &message) != 0) {
        log_error(log, "Could not read a message from Discord: %s",
                  cws_get_error_string(cws));
        goto error;
    }

    Json_Result result = parse_json_value_region_sv(
                             memory,
                             // TODO(#39): receive_discord_payload_from_websocket assumes that the received WebSocket message has only one chunk
                             cws_message_chunk_to_sv(*message.chunks));
    if (result.is_error) {
        log_error(log, "Could not parse WebSocket message from Discord");
        goto error;
    }

    if (!deserialize_discord_payload(result.value, payload)) {
        log_error(log, "Could not deserialize message from Discord");
        goto error;
    }

    cws_free_message(cws, &message);
    return true;
error:
    cws_free_message(cws, &message);
    return false;
}
