#ifndef TZOZEN_H_
#define TZOZEN_H_

// TODO: https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//   [+] 1. #define LIBRARYNAME_IMPLEMENTATION
//   [+] 2. AVOID DEPENDENCIES
//   [+] 3. AVOID MALLOC
//   [+] 4. ALLOW STATIC IMPLEMENTATION
//   [+] 5. MAKE ACCESSIBLE FROM C
//   [ ] 6. NAMESPACE PRIVATE FUNCTIONS
//   [-] 7. EASY-TO-COMPLY LICENSE
//     > why would you care whether your credit appeared there?
//     Because I could put it to my CV and it would be verifiable by Ctrl+F.
//     Which is important when you are nobody.

// NOTES:
// - Define TZOZEN_NO_STDIO to remove all the code that uses stdio.h

#include <assert.h>
#include <stdint.h>
#include <string.h>

#ifndef TZOZEN_NO_STDIO
#include <stdio.h>
#endif // TZOZEN_NO_STDIO

#ifndef TZOZENDEF
#    ifdef TZOZEN_STATIC
#        define TZOZENDEF static
#    else
#        define TZOZENDEF extern
#    endif
#endif

typedef struct {
    size_t capacity;
    size_t size;
    uint8_t *buffer;
} Tzozen_Memory;

TZOZENDEF Tzozen_Memory tzozen_memory(uint8_t *buffer, size_t capacity);

TZOZENDEF void *memory_alloc(Tzozen_Memory *memory, size_t size);

#define UTF8_CHUNK_CAPACITY 4
typedef struct {
    size_t size;
    uint8_t buffer[UTF8_CHUNK_CAPACITY];
} Utf8_Chunk;

TZOZENDEF Utf8_Chunk utf8_encode_rune(uint32_t rune);
TZOZENDEF int json_get_utf8_char_len(unsigned char ch);

typedef struct  {
    size_t len;
    const char *data;
} Tzozen_Str;

#define TSTR(literal) tzozen_str(sizeof(literal) - 1, literal)

TZOZENDEF Tzozen_Str tzozen_str(size_t len, const char *data);
TZOZENDEF void tzozen_str_chop(Tzozen_Str *s, size_t n);
TZOZENDEF Tzozen_Str tzozen_str_chop_until_char(Tzozen_Str *input, char delim);
TZOZENDEF Tzozen_Str tzozen_str_chop_line(Tzozen_Str *input);
TZOZENDEF int tzozen_str_equal(Tzozen_Str a, Tzozen_Str b);
TZOZENDEF Tzozen_Str tzozen_str_take(Tzozen_Str s, size_t n);
TZOZENDEF Tzozen_Str tzozen_str_drop(Tzozen_Str s, size_t n);
TZOZENDEF int tzozen_str_prefix_of(Tzozen_Str prefix, Tzozen_Str s);
TZOZENDEF Tzozen_Str tzozen_str_trim_begin(Tzozen_Str s);
TZOZENDEF int64_t tzozen_str_stoi64(Tzozen_Str integer);
TZOZENDEF int tzozen_str_clone(Tzozen_Memory *memory, Tzozen_Str string, Tzozen_Str *clone);

TZOZENDEF int32_t json_unhex(char x);
TZOZENDEF int json_isspace(char c);
TZOZENDEF int json_isdigit(char c);

#ifndef JSON_DEPTH_MAX_LIMIT
#define JSON_DEPTH_MAX_LIMIT 100
#endif

typedef struct Json_Value Json_Value;

typedef enum {
    JSON_NULL = 0,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} Json_Type;

TZOZENDEF const char *json_type_as_cstr(Json_Type type);

typedef struct Json_Array_Elem Json_Array_Elem;

typedef struct {
    Json_Array_Elem *begin;
    Json_Array_Elem *end;
} Json_Array;

TZOZENDEF size_t json_array_size(Json_Array array);
TZOZENDEF int json_array_push(Tzozen_Memory *memory, Json_Array *array, Json_Value value);

typedef struct Json_Object_Elem Json_Object_Elem;

typedef struct {
    Json_Object_Elem *begin;
    Json_Object_Elem *end;
} Json_Object;

TZOZENDEF size_t json_object_size(Json_Object object);
TZOZENDEF int json_object_push(Tzozen_Memory *memory, Json_Object *object, Tzozen_Str key, Json_Value value);
TZOZENDEF Json_Value json_object_value_by_key(Json_Object object, Tzozen_Str key);

typedef struct {
    // TODO: because of the use of Tzozen_Str-s Json_Number can hold an incorrect value
    //   But you can only get an incorrect Json_Number if you construct it yourself.
    //   Anything coming from parse_json_value should be always a correct number.
    Tzozen_Str integer;
    Tzozen_Str fraction;
    Tzozen_Str exponent;
} Json_Number;

TZOZENDEF int64_t json_number_to_integer(Json_Number number);

struct Json_Value {
    Json_Type type;
    union
    {
        int boolean;
        Json_Number number;
        Tzozen_Str string;
        Json_Array array;
        Json_Object object;
    };
};

TZOZENDEF Json_Value json_null();
TZOZENDEF Json_Value json_true();
TZOZENDEF Json_Value json_false();
TZOZENDEF Json_Value json_number(Tzozen_Str integer, Tzozen_Str fraction, Tzozen_Str exponent);
TZOZENDEF Json_Value json_string(Tzozen_Str string);
TZOZENDEF Json_Value json_array_empty();
TZOZENDEF Json_Value json_array(Json_Array array);
TZOZENDEF Json_Value json_object_empty();
TZOZENDEF Json_Value json_object(Json_Object object);

struct Json_Array_Elem {
    Json_Array_Elem *next;
    Json_Value value;
};

struct Json_Object_Elem {
    Json_Object_Elem *next;
    Tzozen_Str key;
    Json_Value value;
};

#define FOR_JSON(container_type, elem, container)                       \
    for (container_type##_Elem *elem = (container).begin;               \
         elem != NULL;                                                  \
         elem = elem->next)

typedef struct {
    Json_Value value;
    Tzozen_Str rest;
    int is_error;
    const char *message;
} Json_Result;

TZOZENDEF Json_Result result_success(Tzozen_Str rest, Json_Value value);
TZOZENDEF Json_Result result_failure(Tzozen_Str rest, const char *message);


TZOZENDEF Json_Result parse_token(Tzozen_Str source, Tzozen_Str token, Json_Value value, const char *message);
TZOZENDEF Json_Result parse_json_number(Tzozen_Memory *memory, Tzozen_Str source);
TZOZENDEF Json_Result parse_escape_sequence(Tzozen_Memory *memory, Tzozen_Str source);
TZOZENDEF Json_Result parse_json_string_literal(Tzozen_Str source);
TZOZENDEF Json_Result parse_json_string(Tzozen_Memory *memory, Tzozen_Str source);
TZOZENDEF Json_Result parse_json_array(Tzozen_Memory *memory, Tzozen_Str source, int level);
TZOZENDEF Json_Result parse_json_object(Tzozen_Memory *memory, Tzozen_Str source, int level);
TZOZENDEF Json_Result parse_json_value_with_depth(Tzozen_Memory *memory, Tzozen_Str source, int level);
TZOZENDEF Json_Result parse_json_value(Tzozen_Memory *memory, Tzozen_Str source);

#ifndef TZOZEN_NO_STDIO
TZOZENDEF void print_json_null(FILE *stream);
TZOZENDEF void print_json_boolean(FILE *stream, int boolean);
TZOZENDEF void print_json_number(FILE *stream, Json_Number number);
TZOZENDEF void print_json_string(FILE *stream, Tzozen_Str string);
TZOZENDEF void print_json_array(FILE *stream, Json_Array array);
TZOZENDEF void print_json_object(FILE *stream, Json_Object object);
TZOZENDEF void print_json_value(FILE *stream, Json_Value value);
TZOZENDEF void print_json_error(FILE *stream, Json_Result result, Tzozen_Str source, const char *prefix);
#endif // TZOZEN_NO_STDIO

#endif  // TZOZEN_H_

#ifdef TZOZEN_IMPLEMENTATION
// TODO: port https://github.com/tsoding/skedudle/pull/74 when it's merged

TZOZENDEF Tzozen_Memory tzozen_memory(uint8_t *buffer, size_t capacity)
{
    Tzozen_Memory memory;
    memset(&memory, 0, sizeof(memory));
    memory.buffer = buffer;
    memory.capacity = capacity;
    return memory;
}

TZOZENDEF void *memory_alloc(Tzozen_Memory *memory, size_t size)
{
    assert(memory);
    
    if (memory->size + size > memory->capacity) {
        return NULL;
    }

    void *result = memory->buffer + memory->size;
    memory->size += size;

    return result;
}

TZOZENDEF Tzozen_Str tzozen_str(size_t len, const char *data)
{
    Tzozen_Str result = {len, data};
    return result;
}

TZOZENDEF Tzozen_Str tzozen_str_chop_until_char(Tzozen_Str *input, char delim)
{
    if (input->len == 0) {
        return *input;
    }

    size_t i = 0;
    while (i < input->len && input->data[i] != delim)
        ++i;

    Tzozen_Str line;
    line.data = input->data;
    line.len = i;

    if (i == input->len) {
        input->data += input->len;
        input->len = 0;
    } else {
        input->data += i + 1;
        input->len -= i + 1;
    }

    return line;
}

TZOZENDEF Tzozen_Str tzozen_str_chop_line(Tzozen_Str *input)
{
    return tzozen_str_chop_until_char(input, '\n');
}

TZOZENDEF int tzozen_str_equal(Tzozen_Str a, Tzozen_Str b)
{
    if (a.len != b.len) return 0;
    return memcmp(a.data, b.data, a.len) == 0;
}

TZOZENDEF Tzozen_Str tzozen_str_take(Tzozen_Str s, size_t n)
{
    if (s.len < n) return s;
    Tzozen_Str result = { n, s.data };
    return result;
}

TZOZENDEF Tzozen_Str tzozen_str_drop(Tzozen_Str s, size_t n)
{
    if (s.len < n) return TSTR("");
    Tzozen_Str result = {
        s.len - n,
        s.data + n
    };
    return result;
}

TZOZENDEF int tzozen_str_prefix_of(Tzozen_Str prefix, Tzozen_Str s)
{
    return tzozen_str_equal(prefix, tzozen_str_take(s, prefix.len));
}

TZOZENDEF void tzozen_str_chop(Tzozen_Str *s, size_t n)
{
    *s = tzozen_str_drop(*s, n);
}

TZOZENDEF const char *json_type_as_cstr(Json_Type type)
{
    switch (type) {
    case JSON_NULL: return "JSON_NULL";
    case JSON_BOOLEAN: return "JSON_BOOLEAN";
    case JSON_NUMBER: return "JSON_NUMBER";
    case JSON_STRING: return "JSON_STRING";
    case JSON_ARRAY: return "JSON_ARRAY";
    case JSON_OBJECT: return "JSON_OBJECT";
    }

    assert(0 && "Incorrect Json_Type");
    return NULL;
}

TZOZENDEF size_t json_array_size(Json_Array array)
{
    size_t size = 0;
    FOR_JSON (Json_Array, elem, array) size += 1;
    return size;
}

TZOZENDEF size_t json_object_size(Json_Object object)
{
    size_t size = 0;
    FOR_JSON (Json_Object, elem, object) size += 1;
    return size;
}


TZOZENDEF Json_Value json_null()
{
    Json_Value value;
    memset(&value, 0, sizeof(value));
    return value;
}

TZOZENDEF Json_Value json_true()
{
    Json_Value value;
    memset(&value, 0, sizeof(value));
    value.type = JSON_BOOLEAN;
    value.boolean = 1;
    return value;
}

TZOZENDEF Json_Value json_false()
{
    Json_Value value;
    memset(&value, 0, sizeof(value));
    value.type = JSON_BOOLEAN;
    return value;
}

TZOZENDEF Json_Value json_number(Tzozen_Str integer, Tzozen_Str fraction, Tzozen_Str exponent)
{
    Json_Value value = json_null();
    value.type = JSON_NUMBER;
    value.number.integer = integer;
    value.number.fraction = fraction;
    value.number.exponent = exponent;
    return value;
}

TZOZENDEF Json_Value json_string(Tzozen_Str string)
{
    Json_Value value;
    memset(&value, 0, sizeof(value));
    value.type = JSON_STRING;
    value.string = string;
    return value;
}

TZOZENDEF Json_Value json_array_empty()
{
    Json_Value value = json_null();
    value.type = JSON_ARRAY;
    return value;
}

TZOZENDEF Json_Value json_object_empty()
{
    Json_Value value = json_null();
    value.type = JSON_OBJECT;
    return value;
}

TZOZENDEF Json_Value json_array(Json_Array array)
{
    Json_Value value = json_null();
    value.type = JSON_ARRAY;
    value.array = array;
    return value;
}

TZOZENDEF Json_Value json_object(Json_Object object)
{
    Json_Value value = json_null();
    value.type = JSON_OBJECT;
    value.object = object;
    return value;
}

TZOZENDEF int json_isdigit(char c)
{
    return '0' <= c && c <= '9';
}

TZOZENDEF int json_isspace(char c)
{
    return c == 0x20 || c == 0x0A || c == 0x0D || c == 0x09;
}

TZOZENDEF Tzozen_Str tzozen_str_trim_begin(Tzozen_Str s)
{
    while (s.len && json_isspace(*s.data)) {
        s.data++;
        s.len--;
    }
    return s;
}

TZOZENDEF int json_array_push(Tzozen_Memory *memory, Json_Array *array, Json_Value value)
{
    Json_Array_Elem *next = (Json_Array_Elem *) memory_alloc(memory, sizeof(Json_Array_Elem));
    if (next == NULL) {
        return -1;
    }

    memset(next, 0, sizeof(Json_Array_Elem));
    next->value = value;

    if (array->begin == NULL) {
        assert(array->end == NULL);
        array->begin = next;
    } else {
        assert(array->end != NULL);
        array->end->next = next;
    }

    array->end = next;

    return 0;
}

TZOZENDEF int json_object_push(Tzozen_Memory *memory, Json_Object *object, Tzozen_Str key, Json_Value value)
{
    Json_Object_Elem *next = (Json_Object_Elem *) memory_alloc(memory, sizeof(Json_Object_Elem));
    if (next == NULL) {
        return -1;
    }
    memset(next, 0, sizeof(Json_Object_Elem));
    next->key = key;
    next->value = value;

    if (object->begin == NULL) {
        assert(object->end == NULL);
        object->begin = next;
    } else {
        assert(object->end != NULL);
        object->end->next = next;
    }

    object->end = next;
    return 0;
}

TZOZENDEF int64_t tzozen_str_stoi64(Tzozen_Str integer)
{
    if (integer.len == 0) {
        return 0;
    }

    int64_t result = 0;
    int64_t sign = 1;

    if (*integer.data == '-') {
        sign = -1;
        tzozen_str_chop(&integer, 1);
    } else if (*integer.data == '+') {
        sign = 1;
        tzozen_str_chop(&integer, 1);
    }

    while (integer.len) {
        assert(json_isdigit(*integer.data));
        result = result * 10 + (*integer.data - '0');
        tzozen_str_chop(&integer, 1);
    }

    return result * sign;
}

TZOZENDEF int64_t json_number_to_integer(Json_Number number)
{
    int64_t exponent = tzozen_str_stoi64(number.exponent);
    int64_t result = tzozen_str_stoi64(number.integer);

    if (exponent > 0) {
        int64_t sign = result >= 0 ? 1 : -1;

        for (; exponent > 0; exponent -= 1) {
            int64_t x = 0;

            if (number.fraction.len) {
                x = *number.fraction.data - '0';
                tzozen_str_chop(&number.fraction, 1);
            }

            result = result * 10 + sign * x;
        }
    }

    for (; exponent < 0 && result; exponent += 1) {
        result /= 10;
    }

    return result;
}

TZOZENDEF Json_Result result_success(Tzozen_Str rest, Json_Value value)
{
    Json_Result result;
    memset(&result, 0, sizeof(result));
    result.value = value;
    result.rest = rest;
    return result;
}

TZOZENDEF Json_Result result_failure(Tzozen_Str rest, const char *message)
{
    Json_Result result;
    memset(&result, 0, sizeof(result));
    result.is_error = 1;
    result.rest = rest;
    result.message = message;
    return result;
}

TZOZENDEF Json_Result parse_token(Tzozen_Str source, Tzozen_Str token,
                        Json_Value value,
                        const char *message)
{
    if (tzozen_str_equal(tzozen_str_take(source, token.len), token)) {
        return result_success(tzozen_str_drop(source, token.len), value);
    }

    return result_failure(source, message);
}

TZOZENDEF int tzozen_str_clone(Tzozen_Memory *memory, Tzozen_Str string, Tzozen_Str *clone)
{
    char *clone_data = (char *)memory_alloc(memory, string.len);
    if (clone_data == NULL) {
        return -1;
    }

    memcpy(clone_data, string.data, string.len);

    clone->len = string.len;
    clone->data = clone_data;

    return 0;
}

TZOZENDEF Json_Result parse_json_number(Tzozen_Memory *memory, Tzozen_Str source)
{
    Tzozen_Str integer = {0, NULL};
    Tzozen_Str fraction = {0, NULL};
    Tzozen_Str exponent = {0, NULL};

    integer.data = source.data;

    if (source.len && *source.data == '-') {
        integer.len += 1;
        tzozen_str_chop(&source, 1);
    }

    while (source.len && json_isdigit(*source.data)) {
        integer.len += 1;
        tzozen_str_chop(&source, 1);
    }

    // TODO: empty integer with fraction is not taken into account
    if (integer.len == 0
        || tzozen_str_equal(integer, TSTR("-"))
        || (integer.len > 1 && *integer.data == '0')
        || (integer.len > 2 && tzozen_str_prefix_of(TSTR("-0"), integer))) {
        return result_failure(source, "Incorrect number literal");
    }

    if (source.len && *source.data == '.') {
        tzozen_str_chop(&source, 1);
        fraction.data = source.data;

        while (source.len && json_isdigit(*source.data)) {
            fraction.len  += 1;
            tzozen_str_chop(&source, 1);
        }

        if (fraction.len == 0) {
            return result_failure(source, "Incorrect number literal");
        }
    }

    if (source.len && (*source.data == 'e' || *source.data == 'E')) {
        tzozen_str_chop(&source, 1);

        exponent.data = source.data;

        if (source.len && (*source.data == '-' || *source.data == '+')) {
            exponent.len  += 1;
            tzozen_str_chop(&source, 1);
        }

        while (source.len && json_isdigit(*source.data)) {
            exponent.len  += 1;
            tzozen_str_chop(&source, 1);
        }

        if (exponent.len == 0 ||
            tzozen_str_equal(exponent, TSTR("-")) ||
            tzozen_str_equal(exponent, TSTR("+"))) {
            return result_failure(source, "Incorrect number literal");
        }
    }

    Tzozen_Str integer_clone = {0, NULL};
    if (tzozen_str_clone(memory, integer, &integer_clone) < 0){
        return result_failure(source, "Out of memory");
    }

    Tzozen_Str fraction_clone = {0, NULL};
    if (tzozen_str_clone(memory, fraction, &fraction_clone) < 0){
        return result_failure(source, "Out of memory");
    }

    Tzozen_Str exponent_clone = {0, NULL};
    if (tzozen_str_clone(memory, exponent, &exponent_clone) < 0){
        return result_failure(source, "Out of memory");
    }

    return result_success(
        source,
        json_number(
            integer_clone,
            fraction_clone,
            exponent_clone));
}

TZOZENDEF Json_Result parse_json_string_literal(Tzozen_Str source)
{
    if (source.len == 0 || *source.data != '"') {
        return result_failure(source, "Expected '\"'");
    }

    tzozen_str_chop(&source, 1);

    Tzozen_Str s = { 0, source.data };

    while (source.len && *source.data != '"') {
        if (*source.data == '\\') {
            s.len++;
            tzozen_str_chop(&source, 1);

            if (source.len == 0) {
                return result_failure(source, "Unfinished escape sequence");
            }
        }

        s.len++;
        tzozen_str_chop(&source, 1);
    }

    if (source.len == 0) {
        return result_failure(source, "Expected '\"'");
    }

    tzozen_str_chop(&source, 1);

    return result_success(source, json_string(s));
}

TZOZENDEF int32_t json_unhex(char x)
{
    if ('0' <= x && x <= '9') {
        return x - '0';
    } else if ('a' <= x && x <= 'f') {
        return x - 'a' + 10;
    } else if ('A' <= x && x <= 'F') {
        return x - 'A' + 10;
    }

    return -1;
}

TZOZENDEF Utf8_Chunk utf8_encode_rune(uint32_t rune)
{
    const uint8_t b00000111 = (1 << 3) - 1;
    const uint8_t b00001111 = (1 << 4) - 1;
    const uint8_t b00011111 = (1 << 5) - 1;
    const uint8_t b00111111 = (1 << 6) - 1;
    const uint8_t b10000000 = ~((1 << 7) - 1);
    const uint8_t b11000000 = ~((1 << 6) - 1);
    const uint8_t b11100000 = ~((1 << 5) - 1);
    const uint8_t b11110000 = ~((1 << 4) - 1);

    Utf8_Chunk chunk;
    memset(&chunk, 0, sizeof(chunk));

    if (rune <= 0x007F) {
        chunk.size = 1;
        chunk.buffer[0] = (uint8_t) rune;
    } else if (0x0080 <= rune && rune <= 0x07FF) {
        chunk.size = 2;
        chunk.buffer[0] = (uint8_t) (((rune >> 6) & b00011111) | b11000000);
        chunk.buffer[1] = (uint8_t) ((rune        & b00111111) | b10000000);
    } else if (0x0800 <= rune && rune <= 0xFFFF) {
        chunk.size = 3;
        chunk.buffer[0] = (uint8_t) (((rune >> 12) & b00001111) | b11100000);
        chunk.buffer[1] = (uint8_t) (((rune >> 6)  & b00111111) | b10000000);
        chunk.buffer[2] = (uint8_t) ((rune         & b00111111) | b10000000);
    } else if (0x10000 <= rune && rune <= 0x10FFFF) {
        chunk.size = 4;
        chunk.buffer[0] = (uint8_t) (((rune >> 18) & b00000111) | b11110000);
        chunk.buffer[1] = (uint8_t) (((rune >> 12) & b00111111) | b10000000);
        chunk.buffer[2] = (uint8_t) (((rune >> 6)  & b00111111) | b10000000);
        chunk.buffer[3] = (uint8_t) ((rune         & b00111111) | b10000000);
    }

    return chunk;
}

TZOZENDEF Json_Result parse_escape_sequence(Tzozen_Memory *memory, Tzozen_Str source)
{
    static char unescape_map[][2] = {
        {'b', '\b'},
        {'f', '\f'},
        {'n', '\n'},
        {'r', '\r'},
        {'t', '\t'},
        {'/', '/'},
        {'\\', '\\'},
        {'"', '"'},
    };
    static const size_t unescape_map_size =
        sizeof(unescape_map) / sizeof(unescape_map[0]);

    if (source.len == 0 || *source.data != '\\') {
        return result_failure(source, "Expected '\\'");
    }
    tzozen_str_chop(&source, 1);

    if (source.len <= 0) {
        return result_failure(source, "Unfinished escape sequence");
    }

    for (size_t i = 0; i < unescape_map_size; ++i) {
        if (unescape_map[i][0] == *source.data) {
            // It may look like we are refering to something outside
            // of `memory`, but later (see the places where
            // `parse_escape_sequence()` is used 2020-05-05) we are
            // copying it to `memory`.
            //
            // TODO: We don't have any policy on what kind of memory we should always refer to int Json_Value-s and Json_Result-s
            Tzozen_Str s = {1, &unescape_map[i][1]};
            return result_success(tzozen_str_drop(source, 1), json_string(s));
        }
    }

    if (*source.data != 'u') {
        return result_failure(source, "Unknown escape sequence");
    }
    tzozen_str_chop(&source, 1);

    if (source.len < 4) {
        return result_failure(source, "Incomplete unicode point escape sequence");
    }

    uint32_t rune = 0;
    for (int i = 0; i < 4; ++i) {
        int32_t x = json_unhex(*source.data);
        if (x < 0) {
            return result_failure(source, "Incorrect hex digit");
        }
        rune = rune * 0x10 + x;
        tzozen_str_chop(&source, 1);
    }

    if (0xD800 <= rune && rune <= 0xDBFF) {
        if (source.len < 6) {
            return result_failure(source, "Unfinished surrogate pair");
        }

        if (*source.data != '\\') {
            return result_failure(source, "Unfinished surrogate pair. Expected '\\'");
        }
        tzozen_str_chop(&source, 1);

        if (*source.data != 'u') {
            return result_failure(source, "Unfinished surrogate pair. Expected 'u'");
        }
        tzozen_str_chop(&source, 1);

        uint32_t surrogate = 0;
        for (int i = 0; i < 4; ++i) {
            int32_t x = json_unhex(*source.data);
            if (x < 0) {
                return result_failure(source, "Incorrect hex digit");
            }
            surrogate = surrogate * 0x10 + x;
            tzozen_str_chop(&source, 1);
        }

        if (!(0xDC00 <= surrogate && surrogate <= 0xDFFF)) {
            return result_failure(source, "Invalid surrogate pair");
        }

        rune = 0x10000 + (((rune - 0xD800) << 10) |(surrogate - 0xDC00));
    }

    if (rune > 0x10FFFF) {
        rune = 0xFFFD;
    }

    Utf8_Chunk utf8_chunk = utf8_encode_rune(rune);
    assert(utf8_chunk.size > 0);

    char *data = (char *) memory_alloc(memory, utf8_chunk.size);
    if (data == NULL) {
        return result_failure(source, "Out of memory");
    }
    memcpy(data, utf8_chunk.buffer, utf8_chunk.size);

    Tzozen_Str s = {utf8_chunk.size, data};
    return result_success(source, json_string(s));
}

TZOZENDEF Json_Result parse_json_string(Tzozen_Memory *memory, Tzozen_Str source)
{
    Json_Result result = parse_json_string_literal(source);
    if (result.is_error) return result;
    assert(result.value.type == JSON_STRING);

    const size_t buffer_capacity = result.value.string.len;
    source = result.value.string;
    Tzozen_Str rest = result.rest;

    char *buffer = (char *)memory_alloc(memory, buffer_capacity);
    if (buffer == NULL) {
        return result_failure(source, "Out of memory");
    }
    size_t buffer_size = 0;

    while (source.len) {
        if (*source.data == '\\') {
            result = parse_escape_sequence(memory, source);
            if (result.is_error) return result;
            assert(result.value.type == JSON_STRING);
            assert(buffer_size + result.value.string.len <= buffer_capacity);
            memcpy(buffer + buffer_size,
                   result.value.string.data,
                   result.value.string.len);
            buffer_size += result.value.string.len;

            source = result.rest;
        } else {
            // TODO: json parser is not aware of the input encoding
            assert(buffer_size < buffer_capacity);
            buffer[buffer_size++] = *source.data;
            tzozen_str_chop(&source, 1);
        }
    }

    Tzozen_Str result_string = {buffer_size, buffer};
    return result_success(rest, json_string(result_string));
}

TZOZENDEF Json_Result parse_json_array(Tzozen_Memory *memory, Tzozen_Str source, int level)
{
    assert(memory);

    if(source.len == 0 || *source.data != '[') {
        return result_failure(source, "Expected '['");
    }

    tzozen_str_chop(&source, 1);

    source = tzozen_str_trim_begin(source);

    if (source.len == 0) {
        return result_failure(source, "Expected ']'");
    } else if(*source.data == ']') {
        return result_success(tzozen_str_drop(source, 1), json_array_empty());
    }

    Json_Array array;
    memset(&array, 0, sizeof(array));

    while(source.len > 0) {
        Json_Result item_result = parse_json_value_with_depth(memory, source, level + 1);
        if(item_result.is_error) {
            return item_result;
        }

        if (json_array_push(memory, &array, item_result.value) < 0) {
            return result_failure(source, "Out of memory");
        }

        source = tzozen_str_trim_begin(item_result.rest);

        if (source.len == 0) {
            return result_failure(source, "Expected ']' or ','");
        }

        if (*source.data == ']') {
            return result_success(tzozen_str_drop(source, 1), json_array(array));
        }

        if (*source.data != ',') {
            return result_failure(source, "Expected ']' or ','");
        }

        source = tzozen_str_trim_begin(tzozen_str_drop(source, 1));
    }

    return result_failure(source, "EOF");
}

TZOZENDEF Json_Result parse_json_object(Tzozen_Memory *memory, Tzozen_Str source, int level)
{
    assert(memory);

    if (source.len == 0 || *source.data != '{') {
        return result_failure(source, "Expected '{'");;
    }

    tzozen_str_chop(&source, 1);

    source = tzozen_str_trim_begin(source);

    if (source.len == 0) {
        return result_failure(source, "Expected '}'");
    } else if (*source.data == '}') {
        return result_success(tzozen_str_drop(source, 1), json_object_empty());;
    }

    Json_Object object;
    memset(&object, 0, sizeof(object));

    while (source.len > 0) {
        source = tzozen_str_trim_begin(source);

        Json_Result key_result = parse_json_string(memory, source);
        if (key_result.is_error) {
            return key_result;
        }
        source = tzozen_str_trim_begin(key_result.rest);

        if (source.len == 0 || *source.data != ':') {
            return result_failure(source, "Expected ':'");
        }

        tzozen_str_chop(&source, 1);

        Json_Result value_result = parse_json_value_with_depth(memory, source, level + 1);
        if (value_result.is_error) {
            return value_result;
        }
        source = tzozen_str_trim_begin(value_result.rest);

        assert(key_result.value.type == JSON_STRING);
        if (json_object_push(memory, &object, key_result.value.string, value_result.value) < 0) {
            return result_failure(source, "Out of memory");
        }

        if (source.len == 0) {
            return result_failure(source, "Expected '}' or ','");
        }

        if (*source.data == '}') {
            return result_success(tzozen_str_drop(source, 1), json_object(object));
        }

        if (*source.data != ',') {
            return result_failure(source, "Expected '}' or ','");
        }

        source = tzozen_str_drop(source, 1);
    }

    return result_failure(source, "EOF");
}

TZOZENDEF Json_Result parse_json_value_with_depth(Tzozen_Memory *memory, Tzozen_Str source, int level)
{
    if (level >= JSON_DEPTH_MAX_LIMIT) {
        return result_failure(source, "Reached the max limit of depth");
    }

    source = tzozen_str_trim_begin(source);

    if (source.len == 0) {
        return result_failure(source, "EOF");
    }

    switch (*source.data) {
    case 'n': return parse_token(source, TSTR("null"), json_null(), "Expected `null`");
    case 't': return parse_token(source, TSTR("true"), json_true(), "Expected `true`");
    case 'f': return parse_token(source, TSTR("false"), json_false(), "Expected `false`");
    case '"': return parse_json_string(memory, source);
    case '[': return parse_json_array(memory, source, level);
    case '{': return parse_json_object(memory, source, level);
    }

    return parse_json_number(memory, source);
}

// TODO: parse_json_value is not aware of input encoding
TZOZENDEF Json_Result parse_json_value(Tzozen_Memory *memory, Tzozen_Str source)
{
    return parse_json_value_with_depth(memory, source, 0);
}

#ifndef TZOZEN_NO_STDIO
TZOZENDEF void print_json_null(FILE *stream)
{
    fprintf(stream, "null");
}

TZOZENDEF void print_json_boolean(FILE *stream, int boolean)
{
    if (boolean) {
        fprintf(stream, "true");
    } else {
        fprintf(stream, "false");
    }
}

TZOZENDEF void print_json_number(FILE *stream, Json_Number number)
{
    fwrite(number.integer.data, 1, number.integer.len, stream);

    if (number.fraction.len > 0) {
        fputc('.', stream);
        fwrite(number.fraction.data, 1, number.fraction.len, stream);
    }

    if (number.exponent.len > 0) {
        fputc('e', stream);
        fwrite(number.exponent.data, 1, number.exponent.len, stream);
    }
}

TZOZENDEF void print_json_string(FILE *stream, Tzozen_Str string)
{
    const char *hex_digits = "0123456789abcdef";
    const char *specials = "btnvfr";
    const char *p = string.data;

    fputc('"', stream);
    size_t cl;
    for (size_t i = 0; i < string.len; i++) {
        unsigned char ch = ((unsigned char *) p)[i];
        if (ch == '"' || ch == '\\') {
            fwrite("\\", 1, 1, stream);
            fwrite(p + i, 1, 1, stream);
        } else if (ch >= '\b' && ch <= '\r') {
            fwrite("\\", 1, 1, stream);
            fwrite(&specials[ch - '\b'], 1, 1, stream);
        } else if (0x20 <= ch && ch <= 0x7F) { // is printable
            fwrite(p + i, 1, 1, stream);
        } else if ((cl = json_get_utf8_char_len(ch)) == 1) {
            fwrite("\\u00", 1, 4, stream);
            fwrite(&hex_digits[(ch >> 4) % 0xf], 1, 1, stream);
            fwrite(&hex_digits[ch % 0xf], 1, 1, stream);
        } else {
            fwrite(p + i, 1, cl, stream);
            i += cl - 1;
        }
    }
    fputc('"', stream);
}

TZOZENDEF void print_json_array(FILE *stream, Json_Array array)
{
    fprintf(stream, "[");
    int t = 0;
    FOR_JSON (Json_Array, elem, array) {
        if (t) {
            fprintf(stream, ",");
        } else {
            t = 1;
        }
        print_json_value(stream, elem->value);
    }
    fprintf(stream, "]");
}

TZOZENDEF void print_json_object(FILE *stream, Json_Object object)
{
    fprintf(stream, "{");
    int t = 0;
    FOR_JSON (Json_Object, elem, object) {
        if (t) {
            fprintf(stream, ",");
        } else {
            t = 1;
        }
        print_json_string(stream, elem->key);
        fprintf(stream, ":");
        print_json_value(stream, elem->value);
    }
    fprintf(stream, "}");
}

TZOZENDEF void print_json_value(FILE *stream, Json_Value value)
{
    switch (value.type) {
    case JSON_NULL: {
        print_json_null(stream);
    } break;
    case JSON_BOOLEAN: {
        print_json_boolean(stream, value.boolean);
    } break;
    case JSON_NUMBER: {
        print_json_number(stream, value.number);
    } break;
    case JSON_STRING: {
        print_json_string(stream, value.string);
    } break;
    case JSON_ARRAY: {
        print_json_array(stream, value.array);
    } break;
    case JSON_OBJECT: {
        print_json_object(stream, value.object);
    } break;
    }
}

TZOZENDEF void print_json_error(FILE *stream, Json_Result result,
                                Tzozen_Str source, const char *prefix)
{
    assert(stream);
    assert(source.data <= result.rest.data);

    size_t n = result.rest.data - source.data;

    for (size_t line_number = 1; source.len; ++line_number) {
        Tzozen_Str line = tzozen_str_chop_line(&source);

        if (n <= line.len) {
            fprintf(stream, "%s:%ld: %s\n", prefix, line_number, result.message);
            fwrite(line.data, 1, line.len, stream);
            fputc('\n', stream);

            for (size_t j = 0; j < n; ++j) {
                fputc(' ', stream);
            }
            fputc('^', stream);
            fputc('\n', stream);
            break;
        }

        n -= line.len + 1;
    }

    for (int i = 0; source.len && i < 3; ++i) {
        Tzozen_Str line = tzozen_str_chop_line(&source);
        fwrite(line.data, 1, line.len, stream);
        fputc('\n', stream);
    }
}
#endif // TZOZEN_NO_STDIO

TZOZENDEF int json_get_utf8_char_len(unsigned char ch)
{
    if ((ch & 0x80) == 0) return 1;
    switch (ch & 0xf0) {
        case 0xf0:
            return 4;
        case 0xe0:
            return 3;
        default:
            return 2;
    }
}

TZOZENDEF Json_Value json_object_value_by_key(Json_Object object, Tzozen_Str key)
{
    FOR_JSON (Json_Object, element, object) {
        if (tzozen_str_equal(element->key, key)) {
            return element->value;
        }
    }
    return json_null();
}

#endif // TZOZEN_IMPLEMENTATION
