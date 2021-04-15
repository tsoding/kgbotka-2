#ifndef JSON_H_
#define JSON_H_

#include "region.h"
#include "thirdparty/tzozen.h"

Json_Result parse_json_value_region_sv(Region *memory, String_View raw_body);

#endif // JSON_H_
