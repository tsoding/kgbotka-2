#ifndef DISCORD_H_
#define DISCORD_H_

#include "./tzozen.h"
#include "./sv.h"

bool extract_discord_gateway_url(Json_Value discord_gateway_response,
                                 String_View *gateway_url);

#endif // DISCORD_H_
