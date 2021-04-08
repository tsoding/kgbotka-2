#include "./discord.h"

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
