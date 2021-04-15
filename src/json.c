#include "json.h"

Json_Result parse_json_value_region_sv(Region *memory, String_View raw_body)
{
    Tzozen_Memory tmemory = region_to_tzozen_memory(memory);
    Json_Result result = parse_json_value(&tmemory, sv_to_tzozen_str(raw_body));
    memory->size = tmemory.size;
    return result;
}
