#define JSONLITE_AMALGAMATED
#include "jsonlite.h"
//
//  Copyright 2012-2014, Andrii Mamchur
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License

const char *jsonlite_version = "1.2.0";
//
//  Copyright 2012-2014, Andrii Mamchur
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License

#ifndef JSONLITE_AMALGAMATED
#include "jsonlite_buffer.h"
#endif

#include <stdlib.h>
#include <string.h>

static int jsonlite_null_buffer_set_append(jsonlite_buffer buffer, const void *data, size_t length) {
    return length == 0 ? 0 : -1;
}

struct jsonlite_buffer_struct jsonlite_null_buffer_struct = {
    NULL,
    0,
    0,
    &jsonlite_null_buffer_set_append,
    &jsonlite_null_buffer_set_append
};

jsonlite_buffer jsonlite_null_buffer = &jsonlite_null_buffer_struct;

const void *jsonlite_buffer_data(jsonlite_buffer buffer) {
    return buffer->mem;
}

size_t jsonlite_buffer_size(jsonlite_buffer buffer) {
    return buffer->size;
}

int jsonlite_buffer_set_mem(jsonlite_buffer buffer, const void *data, size_t length) {
    return buffer->set_mem(buffer, data, length);
}

int jsonlite_buffer_append_mem(jsonlite_buffer buffer, const void *data, size_t length) {
    return buffer->append_mem(buffer, data, length);
}

static int jsonlite_static_buffer_set_mem(jsonlite_buffer buffer, const void *data, size_t length) {
    if (length > buffer->capacity) {
        return -1;
    }

    buffer->size = length;
    memcpy(buffer->mem, data, length);
    return 0;
}

static int jsonlite_static_buffer_append_mem(jsonlite_buffer buffer, const void *data, size_t length) {
    size_t total_size = buffer->size + length;
    if (total_size > buffer->capacity) {
        return -1;
    }

    memcpy(buffer->mem + buffer->size, data, length);
    buffer->size = total_size;
    return 0;
}

jsonlite_buffer jsonlite_static_buffer_init(void *mem, size_t size) {
    struct jsonlite_buffer_struct *buffer = (struct jsonlite_buffer_struct *)mem;
    buffer->set_mem = &jsonlite_static_buffer_set_mem;
    buffer->append_mem = &jsonlite_static_buffer_append_mem;
    buffer->mem = (uint8_t *)mem + sizeof(jsonlite_buffer_struct);
    buffer->size = 0;
    buffer->capacity = size - sizeof(jsonlite_buffer_struct);
    return buffer;
}

static int jsonlite_heap_buffer_set_mem(jsonlite_buffer buffer, const void *data, size_t length) {
    if (length > buffer->capacity) {
        free(buffer->mem);
        buffer->mem = malloc(length);
        buffer->capacity = length;
    }

    buffer->size = length;
    memcpy(buffer->mem, data, length);
    return 0;
}

static int jsonlite_heap_buffer_append_mem(jsonlite_buffer buffer, const void *data, size_t length) {
    size_t total_size = buffer->size + length;
    if (total_size > buffer->capacity) {
        uint8_t *b = (uint8_t *)malloc(total_size);
        memcpy(b, buffer->mem, buffer->size);

        free(buffer->mem);
        buffer->mem = b;
        buffer->capacity = total_size;
    }

    memcpy(buffer->mem + buffer->size, data, length);
    buffer->size = total_size;
    return 0;
}

void jsonlite_heap_buffer_cleanup(jsonlite_buffer buffer) {
    if (buffer != NULL) {
        free(buffer->mem);
        buffer->mem = NULL;
        buffer->size = 0;
        buffer->capacity = 0;
    }
}

jsonlite_buffer jsonlite_heap_buffer_init(void *mem) {
    struct jsonlite_buffer_struct *buffer = (struct jsonlite_buffer_struct *)mem;
    buffer->set_mem = &jsonlite_heap_buffer_set_mem;
    buffer->append_mem = &jsonlite_heap_buffer_append_mem;
    buffer->mem = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
    return buffer;
}
//
//  Copyright 2012-2014, Andrii Mamchur
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License

#ifndef JSONLITE_AMALGAMATED
#include "../include/jsonlite_builder.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#define MIN_DEPTH 2

#define jsonlite_builder_check_depth()          \
do {                                            \
    if (builder->state >= builder->limit) {     \
        return jsonlite_result_depth_limit;     \
    }                                           \
} while (0)

enum {
    jsonlite_accept_object_begin    = 0x0001,
    jsonlite_accept_object_end      = 0x0002,
    jsonlite_accept_array_begin     = 0x0004,
    jsonlite_accept_array_end       = 0x0008,
    jsonlite_accept_key             = 0x0010,
    jsonlite_accept_string          = 0x0020,
    jsonlite_accept_number          = 0x0040,
    jsonlite_accept_boolean         = 0x0080,
    jsonlite_accept_null            = 0x0100,
    jsonlite_accept_values_only     = 0x0200,
    jsonlite_accept_next            = 0x0400,

    jsonlite_accept_value           = 0
                                    | jsonlite_accept_object_begin
                                    | jsonlite_accept_array_begin
                                    | jsonlite_accept_string
                                    | jsonlite_accept_number
                                    | jsonlite_accept_boolean
                                    | jsonlite_accept_null,
    jsonlite_accept_continue_object = 0
                                    | jsonlite_accept_next
                                    | jsonlite_accept_key
                                    | jsonlite_accept_object_end,
    jsonlite_accept_continue_array = 0
                                    | jsonlite_accept_next
                                    | jsonlite_accept_values_only
                                    | jsonlite_accept_value
                                    | jsonlite_accept_array_end
};

static int jsonlite_builder_accept(jsonlite_builder builder, jsonlite_write_state a);
static void jsonlite_builder_pop_state(jsonlite_builder builder);
static void jsonlite_builder_prepare_value_writing(jsonlite_builder builder);
static void jsonlite_builder_raw_char(jsonlite_builder builder, char data);
static void jsonlite_builder_write_uft8(jsonlite_builder builder, const char *data, size_t length);
static void jsonlite_builder_raw(jsonlite_builder builder, const void *data, size_t length);
static void jsonlite_builder_repeat(jsonlite_builder builder, const char ch, size_t count);
static void jsonlite_builder_write_base64(jsonlite_builder builder, const void *data, size_t length);
static jsonlite_builder jsonlite_builder_configure(void *memory, size_t size, jsonlite_stream stream);

jsonlite_builder jsonlite_builder_init(void *memory, size_t size, jsonlite_stream stream) {
    if (size < jsonlite_builder_estimate_size(MIN_DEPTH)) {
        return NULL;
    }

    return jsonlite_builder_configure(memory, size, stream);
}

static jsonlite_builder jsonlite_builder_configure(void *memory, size_t size, jsonlite_stream stream) {
    jsonlite_builder builder = (jsonlite_builder)memory;
    size_t depth = (size - sizeof(jsonlite_builder_struct)) / sizeof(jsonlite_write_state);
    builder->state = (jsonlite_write_state *)((uint8_t *)builder + sizeof(jsonlite_builder_struct));
    builder->limit = builder->state + depth - 1;
    builder->stack = builder->state;
    builder->stream = stream;
    builder->indentation = 0;
    *builder->state = jsonlite_accept_object_begin | jsonlite_accept_array_begin;
    jsonlite_builder_set_double_format(builder, "%.16g");
    return builder;
}

void jsonlite_builder_set_indentation(jsonlite_builder builder, size_t indentation) {
    builder->indentation = indentation;
}

void jsonlite_builder_set_double_format(jsonlite_builder builder, const char *format) {
    strcpy(builder->doubleFormat, format);
}

static int jsonlite_builder_accept(jsonlite_builder builder, jsonlite_write_state a) {
    return (*builder->state & a) == a;
}

static void jsonlite_builder_pop_state(jsonlite_builder builder) {
    jsonlite_write_state *ws = --builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
        *ws = jsonlite_accept_continue_array;
    } else {
        *ws = jsonlite_accept_continue_object;
    }
}

static void jsonlite_builder_prepare_value_writing(jsonlite_builder builder) {
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
        if (jsonlite_builder_accept(builder, jsonlite_accept_next)) {
            jsonlite_builder_raw_char(builder, ',');
        }

        if (builder->indentation != 0) {
            jsonlite_builder_raw_char(builder, '\r');
            jsonlite_builder_repeat(builder, ' ', (builder->state - builder->stack) * builder->indentation);
        }
    } else {
        *ws &= ~jsonlite_accept_value;
        *ws |= jsonlite_accept_key;
    }
    *ws |= jsonlite_accept_next;
}

jsonlite_result jsonlite_builder_object_begin(jsonlite_builder builder) {
    jsonlite_builder_check_depth();

    if (jsonlite_builder_accept(builder, jsonlite_accept_object_begin)) {
        jsonlite_builder_prepare_value_writing(builder);
        *++builder->state = jsonlite_accept_object_end | jsonlite_accept_key;
        jsonlite_builder_raw_char(builder, '{');
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_object_end(jsonlite_builder builder) {
    if (jsonlite_builder_accept(builder, jsonlite_accept_object_end)) {
        jsonlite_builder_pop_state(builder);
        if (builder->indentation != 0) {
            jsonlite_builder_raw_char(builder, '\r');
            jsonlite_builder_repeat(builder, ' ', (builder->state - builder->stack) * builder->indentation);
        }
        jsonlite_builder_raw_char(builder, '}');
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_array_begin(jsonlite_builder builder) {
    jsonlite_builder_check_depth();

    if (jsonlite_builder_accept(builder, jsonlite_accept_array_begin)) {
        jsonlite_builder_prepare_value_writing(builder);
        *++builder->state = jsonlite_accept_array_end
            | jsonlite_accept_value
            | jsonlite_accept_values_only;
        jsonlite_builder_raw_char(builder, '[');
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_array_end(jsonlite_builder builder) {
    if (jsonlite_builder_accept(builder, jsonlite_accept_array_end)) {
        jsonlite_builder_pop_state(builder);
        if (builder->indentation != 0) {
            jsonlite_builder_raw_char(builder, '\r');
            jsonlite_builder_repeat(builder, ' ', (builder->state - builder->stack) * builder->indentation);
        }
        jsonlite_builder_raw_char(builder, ']');
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

static void jsonlite_builder_write_uft8(jsonlite_builder builder, const char *data, size_t length) {
    char b[2] = {'\\', '?'};
    const char *c = data;
    const char *p = data;
    const char *l = data + length;

    jsonlite_builder_raw_char(builder, '\"');
next:
    if (c == l)                     goto end;
    switch (*c) {
        case '"':   b[1] = '"';     goto flush;
        case '\\':  b[1] = '\\';    goto flush;
        case '\b':  b[1] = 'b';     goto flush;
        case '\f':  b[1] = 'f';     goto flush;
        case '\n':  b[1] = 'n';     goto flush;
        case '\r':  b[1] = 'r';     goto flush;
        case '\t':  b[1] = 't';     goto flush;
        default:    c++;            goto next;
    }
flush:
    jsonlite_stream_write(builder->stream, p, c - p);
    jsonlite_stream_write(builder->stream, b, 2);
    p = ++c;
    goto next;
end:
    jsonlite_stream_write(builder->stream, p, c - p);
    jsonlite_builder_raw_char(builder, '\"');
}

jsonlite_result jsonlite_builder_key(jsonlite_builder builder, const char *data, size_t length) {
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_key)) {
        if (jsonlite_builder_accept(builder, jsonlite_accept_next)) {
            jsonlite_builder_raw_char(builder, ',');
        }

        if (builder->indentation != 0) {
            jsonlite_builder_raw_char(builder, '\r');
            jsonlite_builder_repeat(builder, ' ', (builder->state - builder->stack) * builder->indentation);
        }

        jsonlite_builder_write_uft8(builder, data, length);
        if (builder->indentation != 0) {
            jsonlite_builder_raw(builder, ": ", 2);
        } else {
            jsonlite_builder_raw_char(builder, ':');
        }
        *ws = jsonlite_accept_value;
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_string(jsonlite_builder builder, const char *data, size_t length) {
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_value)) {
        jsonlite_builder_prepare_value_writing(builder);
        jsonlite_builder_write_uft8(builder, data, length);
        if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
            *ws = jsonlite_accept_continue_array;
        } else {
            *ws = jsonlite_accept_continue_object;
        }
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_int(jsonlite_builder builder, long long value) {
    char buff[64];
  size_t size = 0;
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_value)) {
        jsonlite_builder_prepare_value_writing(builder);
        size = sprintf(buff, "%lld", value);
        jsonlite_builder_raw(builder, buff, size);
        if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
            *ws = jsonlite_accept_continue_array;
        } else {
            *ws = jsonlite_accept_continue_object;
        }
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_double(jsonlite_builder builder, double value) {
    char buff[64];
  size_t size = 0;
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_value)) {
        jsonlite_builder_prepare_value_writing(builder);
        size = sprintf(buff, builder->doubleFormat, value);
        jsonlite_builder_raw(builder, buff, size);
        if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
            *ws = jsonlite_accept_continue_array;
        } else {
            *ws = jsonlite_accept_continue_object;
        }
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_true(jsonlite_builder builder) {
    static const char value[] = "true";
  jsonlite_write_state *ws = builder->state;
    if (!(jsonlite_builder_accept(builder, jsonlite_accept_value))) {

        return jsonlite_result_not_allowed;
    }

    jsonlite_builder_prepare_value_writing(builder);
    jsonlite_builder_raw(builder, (char *)value, sizeof(value) - 1);
    if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
        *ws = jsonlite_accept_continue_array;
    } else {
        *ws = jsonlite_accept_continue_object;
    }
    return jsonlite_result_ok;
}

jsonlite_result jsonlite_builder_false(jsonlite_builder builder) {
    static const char value[] = "false";
    jsonlite_write_state *ws = builder->state;
    if (!(jsonlite_builder_accept(builder, jsonlite_accept_value))) {
        return jsonlite_result_not_allowed;
    }

    jsonlite_builder_prepare_value_writing(builder);
    jsonlite_builder_raw(builder, (char *)value, sizeof(value) - 1);
    if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
        *ws = jsonlite_accept_continue_array;
    } else {
        *ws = jsonlite_accept_continue_object;
    }
    return jsonlite_result_ok;
}

jsonlite_result jsonlite_builder_null(jsonlite_builder builder) {
    static const char value[] = "null";
  jsonlite_write_state *ws = builder->state;
    if (!(jsonlite_builder_accept(builder, jsonlite_accept_value))) {

        return jsonlite_result_not_allowed;
    }

    jsonlite_builder_prepare_value_writing(builder);
    jsonlite_builder_raw(builder, (char *)value, sizeof(value) - 1);
    if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
        *ws = jsonlite_accept_continue_array;
    } else {
        *ws = jsonlite_accept_continue_object;
    }
    return jsonlite_result_ok;
}

static void jsonlite_builder_raw(jsonlite_builder builder, const void *data, size_t length) {
    jsonlite_stream_write(builder->stream, data, length);
}

static void jsonlite_builder_repeat(jsonlite_builder builder, const char ch, size_t count) {
    ptrdiff_t i = 0;
    for (; i < count; i++) {
        jsonlite_stream_write(builder->stream, &ch, 1);
    }
}

static void jsonlite_builder_raw_char(jsonlite_builder builder, char data) {
    jsonlite_stream_write(builder->stream, &data, 1);
}

jsonlite_result jsonlite_builder_raw_key(jsonlite_builder builder, const void *data, size_t length) {
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_key)) {
        if (jsonlite_builder_accept(builder, jsonlite_accept_next)) {
            jsonlite_builder_raw(builder, ",", 1);
        }

        if (builder->indentation != 0) {
            jsonlite_builder_raw_char(builder, '\r');
            jsonlite_builder_repeat(builder, ' ', (builder->state - builder->stack) * builder->indentation);
        }

        jsonlite_builder_raw_char(builder, '\"');
        jsonlite_builder_raw(builder, data, length);
        jsonlite_builder_raw_char(builder, '\"');
        if (builder->indentation != 0) {
            jsonlite_builder_raw(builder, ": ", 2);
        } else {
            jsonlite_builder_raw_char(builder, ':');
        }
        *ws = jsonlite_accept_value;
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_raw_string(jsonlite_builder builder, const void *data, size_t length) {
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_value)) {
        jsonlite_builder_prepare_value_writing(builder);
        jsonlite_builder_raw_char(builder, '\"');
        jsonlite_builder_raw(builder, data, length);
        jsonlite_builder_raw_char(builder, '\"');
        if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
            *ws = jsonlite_accept_continue_array;
        } else {
            *ws = jsonlite_accept_continue_object;
        }
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

jsonlite_result jsonlite_builder_raw_value(jsonlite_builder builder, const void *data, size_t length) {
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_value)) {
        jsonlite_builder_prepare_value_writing(builder);
        jsonlite_builder_raw(builder, data, length);
        if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
            *ws = jsonlite_accept_continue_array;
        } else {
            *ws = jsonlite_accept_continue_object;
        }
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}

static void jsonlite_builder_write_base64(jsonlite_builder builder, const void *data, size_t length) {
    static const char encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char buffer[5] = {0};
    const uint8_t *c = data;
    const uint8_t *l = data + length;
    uint32_t bits;
    jsonlite_stream_write(builder->stream, "\"", 1);
next:
    switch (l - c) {
        case 0:
            goto done;
        case 1:
            bits = *c++ << 16;
            buffer[0] = encode[(bits & 0x00FC0000) >> 18];
            buffer[1] = encode[(bits & 0x0003F000) >> 12];
            buffer[2] = '=';
            buffer[3] = '=';
            l = c;
            goto write;
        case 2:
            bits = *c++ << 16;
            bits |= *c++ << 8;
            buffer[0] = encode[(bits & 0x00FC0000) >> 18];
            buffer[1] = encode[(bits & 0x0003F000) >> 12];
            buffer[2] = encode[(bits & 0x00000FC0) >> 6];
            buffer[3] = '=';
            l = c;
            goto write;
        default:
            bits = *c++ << 16;
            bits |= *c++ << 8;
            bits |= *c++;
            buffer[0] = encode[(bits & 0x00FC0000) >> 18];
            buffer[1] = encode[(bits & 0x0003F000) >> 12];
            buffer[2] = encode[(bits & 0x00000FC0) >> 6];
            buffer[3] = encode[(bits & 0x0000003F)];
            goto write;
    }
write:
    jsonlite_stream_write(builder->stream, buffer, 4);
    goto next;
done:
    jsonlite_stream_write(builder->stream, "\"", 1);
}

jsonlite_result jsonlite_builder_base64_value(jsonlite_builder builder, const void *data, size_t length) {
    jsonlite_write_state *ws = builder->state;
    if (jsonlite_builder_accept(builder, jsonlite_accept_value)) {
        jsonlite_builder_prepare_value_writing(builder);
        jsonlite_builder_write_base64(builder, data, length);
        if (jsonlite_builder_accept(builder, jsonlite_accept_values_only)) {
            *ws = jsonlite_accept_continue_array;
        } else {
            *ws = jsonlite_accept_continue_object;
        }
        return jsonlite_result_ok;
    }

    return jsonlite_result_not_allowed;
}
//
//  Copyright 2012-2014, Andrii Mamchur
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License

#ifndef JSONLITE_AMALGAMATED
#include "jsonlite_parser.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER

#include <intrin.h>

static uint32_t __inline jsonlite_clz(uint32_t x) {
   unsigned long r = 0;
   _BitScanForward(&r, x);
   return r;
}

#else

#define jsonlite_clz(x) __builtin_clz((x))

#endif

#define MIN_DEPTH 2
#define CALL_VALUE_CALLBACK(cbs, type, token)   (cbs.type(&cbs.context, token))
#define CALL_STATE_CALLBACK(cbs, type)          (cbs.type(&cbs.context))

#define HEX_CHAR_TO_INT(c, step)        \
if (0x30 <= *c && *c <= 0x39) {         \
    hex_value = *c - 0x30;              \
} else if (0x41 <= *c && *c <= 0x46) {  \
    hex_value = *c - 0x37;              \
} else if (0x61 <= *c && *c <= 0x66) {  \
    hex_value = *c - 0x57;              \
} else goto error_escape;               \
c += step

#define CASE_NUMBER_TOKEN_END   \
case 0x09: goto number_parsed;  \
case 0x0A: goto number_parsed;  \
case 0x0D: goto number_parsed;  \
case 0x20: goto number_parsed;  \
case 0x2C: goto number_parsed;  \
case 0x5D: goto number_parsed;  \
case 0x7D: goto number_parsed

enum {
    state_start,
    state_object_key,
    state_object_key_only,
    state_object_key_or_end,
    state_object_colon,
    state_object_comma_or_end,
    state_array_value_or_end,
    state_array_comma_or_end,
    state_value,
    state_end,

    state_stop = 1 << 7
};

static void jsonlite_do_parse(jsonlite_parser parser);
static void empty_value_callback(jsonlite_callback_context *ctx, jsonlite_token *t) {}
static void empty_state_callback(jsonlite_callback_context *ctx) {}

const jsonlite_parser_callbacks jsonlite_default_callbacks = {
    &empty_state_callback,
    &empty_state_callback,
    &empty_state_callback,
    &empty_state_callback,
    &empty_state_callback,
    &empty_state_callback,
    &empty_state_callback,
    &empty_state_callback,
    &empty_value_callback,
    &empty_value_callback,
    &empty_value_callback,
    {NULL, NULL}
};

static jsonlite_parser jsonlite_parser_configure(void *memory, size_t size, jsonlite_buffer rest_buffer) {
    size_t depth = (size - sizeof(jsonlite_parser_struct)) / sizeof(parse_state);
    jsonlite_parser parser = (jsonlite_parser)memory;
    parser->result = jsonlite_result_unknown;
    parser->rest_buffer = rest_buffer;
    parser->callbacks = jsonlite_default_callbacks;
    parser->control = NULL;
    parser->current = ((uint8_t *)parser + sizeof(jsonlite_parser_struct));
    parser->current[0] = state_end;
    parser->current[1] = state_start;
    parser->last = parser->current + depth;
    parser->current++;
    return parser;
}

jsonlite_parser jsonlite_parser_init(void *memory, size_t size, jsonlite_buffer rest_buffer) {
    if (size < jsonlite_parser_estimate_size(MIN_DEPTH)) {
        return NULL;
    }

    return jsonlite_parser_configure(memory, size, rest_buffer);
}

void jsonlite_parser_set_callback(jsonlite_parser parser, const jsonlite_parser_callbacks *cbs) {
    parser->callbacks = *cbs;
    parser->callbacks.context.parser = parser;
}

jsonlite_result jsonlite_parser_get_result(jsonlite_parser parser) {
    return parser->result;
}

jsonlite_result jsonlite_parser_tokenize(jsonlite_parser parser, const void *buffer, size_t size) {
    size_t rest_size = jsonlite_buffer_size(parser->rest_buffer);
    if (rest_size > 0) {
        if (jsonlite_buffer_append_mem(parser->rest_buffer, buffer, size) < 0) {
            return jsonlite_result_out_of_memory;
        }

        parser->buffer = jsonlite_buffer_data(parser->rest_buffer);
        parser->cursor = parser->buffer;
        parser->limit = parser->buffer + jsonlite_buffer_size(parser->rest_buffer);
    } else {
        parser->buffer = buffer;
        parser->cursor = parser->buffer;
        parser->limit = parser->buffer + size;
    }

    jsonlite_do_parse(parser);
    return parser->result;
}

jsonlite_result jsonlite_parser_resume(jsonlite_parser parser) {
    if (parser->result != jsonlite_result_suspended) {
        return jsonlite_result_not_allowed;
    }

    jsonlite_do_parse(parser);
    return parser->result;
}

jsonlite_result jsonlite_parser_suspend(jsonlite_parser parser) {
    return jsonlite_parser_terminate(parser, jsonlite_result_suspended);
}

jsonlite_result jsonlite_parser_terminate(jsonlite_parser parser, jsonlite_result result) {
    if (parser->control == NULL) {
        return jsonlite_result_not_allowed;
    }

    parser->result = result;
    **parser->control |= state_stop;
    return jsonlite_result_ok;
}

static void jsonlite_do_parse(jsonlite_parser parser) {
    const uint8_t *c = parser->cursor;
    const uint8_t *l = parser->limit;
    const uint8_t *token_start = NULL;
    const parse_state *last = parser->last;
    parse_state *state = parser->current;
    jsonlite_token token = {NULL, NULL, NULL, 0};
    jsonlite_result result = jsonlite_result_ok;
    uint32_t value, utf32;
    uint8_t hex_value;

    *state &= ~state_stop;
    parser->control = &state;
    goto select_state;

structure_finished:
    if (*state == state_end)            goto end;
skip_char_and_whitespaces:
    c++;
select_state:
    if (c == l)     goto end_of_stream_whitespaces;
    if (*c == 0x20) goto skip_char_and_whitespaces;
    if (*c == 0x0A) goto skip_char_and_whitespaces;
    if (*c == 0x0D) goto skip_char_and_whitespaces;
    if (*c == 0x09) goto skip_char_and_whitespaces;
    token_start = c;

    switch (*state) {
        case state_value:               goto parse_value;
        case state_object_key:          goto parse_string;
        case state_object_key_only:     goto parse_object_key_only;
        case state_object_key_or_end:   goto parse_object_key_or_end;
        case state_object_colon:        goto parse_colon;
        case state_object_comma_or_end: goto parse_object_comma_end;
        case state_array_comma_or_end:  goto parse_array_comma_end;
        case state_array_value_or_end:  goto parse_array_value_end;
        case state_start:
            if (*c == 0x7B)             goto parse_object;
            if (*c == 0x5B)             goto parse_array_state;
            goto error_exp_ooa;
        case state_end:                 goto end;
        default:
            result = parser->result;
            goto end;
    }
parse_object:
    *state = state_object_key_or_end;
    CALL_STATE_CALLBACK(parser->callbacks, object_start);
    goto skip_char_and_whitespaces;
parse_array_state:
    *state = state_array_value_or_end;
    CALL_STATE_CALLBACK(parser->callbacks, array_start);
    goto skip_char_and_whitespaces;
parse_colon:
    if (*c != 0x3A) goto error_exp_colon;
    *state = state_object_comma_or_end;
    *++state = state_value;
    goto skip_char_and_whitespaces;
parse_object_comma_end:
    switch (*c) {
        case 0x2C:
            *state = state_object_key_only;
            goto skip_char_and_whitespaces;
        case 0x7D:
            state--;
            CALL_STATE_CALLBACK(parser->callbacks, object_end);
            goto structure_finished;
        default: goto error_exp_coe;
    }
parse_array_value_end:
    switch (*c) {
        case 0x5D:
            state--;
            CALL_STATE_CALLBACK(parser->callbacks, array_end);
            goto structure_finished;
        default:
            *state = state_array_comma_or_end;
            if (++state == last) goto error_depth;
            *state = state_value;
            goto parse_value;
    }
parse_array_comma_end:
    switch (*c) {
        case 0x2C:
            *++state = state_value;
            goto skip_char_and_whitespaces;
        case 0x5D:
            state--;
            CALL_STATE_CALLBACK(parser->callbacks, array_end);
            goto structure_finished;
        default: goto error_exp_coe;
    }
parse_value:
    if (0x31 <= *c && *c <= 0x39)   goto parse_digit_leading_number;
    if (*c == 0x30)                 goto parse_zero_leading_number;
    if (*c == 0x2D)                 goto parse_negative_number;
    if (*c == 0x22)                 goto parse_string;
    if (*c == 0x74)                 goto parse_true_token;
    if (*c == 0x66)                 goto parse_false_token;
    if (*c == 0x6E)                 goto parse_null_token;
    if (*c == 0x7B)                 goto parse_object;
    if (*c == 0x5B)                 goto parse_array_state;
    goto error_exp_value;

// Number parsing
parse_negative_number:
    token.start = c;
    token.type.number = jsonlite_number_int | jsonlite_number_negative;
    if (++c == l)                   goto end_of_stream;
    if (0x31 <= *c && *c <= 0x39)   goto parse_digits;
    if (*c == 0x30)                 goto parse_exponent_fraction;
    goto error_number;
parse_zero_leading_number:
    token.start = c;
    token.type.number = jsonlite_number_int | jsonlite_number_zero_leading;
parse_exponent_fraction:
    if (++c == l)                   goto end_of_stream;
    switch (*c) {
        CASE_NUMBER_TOKEN_END;
        case 0x2E:                  goto parse_fraction;
        case 0x45:                  goto parse_exponent;
        case 0x65:                  goto parse_exponent;
        default:                    goto error_number;
    }
parse_fraction:
    token.type.number |= jsonlite_number_frac;
    if (++c == l)                   goto end_of_stream;
    if (0x30 <= *c && *c <= 0x39)   goto parse_frac_number;
    goto error_number;
parse_frac_number:
    if (++c == l)                   goto end_of_stream;
    if (0x30 <= *c && *c <= 0x39)   goto parse_frac_number;
    switch (*c) {
        CASE_NUMBER_TOKEN_END;
        case 0x45:                  goto parse_exponent;
        case 0x65:                  goto parse_exponent;
        default:                    goto error_number;
    }
parse_exponent:
    token.type.number |= jsonlite_number_exp;
    if (++c == l)                   goto end_of_stream;
    if (0x30 <= *c && *c <= 0x39)   goto parse_exponent_number;
    switch(*c) {
        case 0x2B:                  goto parse_exponent_sign;
        case 0x2D:                  goto parse_exponent_sign;
        default:                    goto error_number;
    }
parse_exponent_sign:
    if (++c == l)                   goto end_of_stream;
    if (0x30 <= *c && *c <= 0x39)   goto parse_exponent_number;
    goto error_number;
parse_exponent_number:
    if (++c == l)                   goto end_of_stream;
    if (0x30 <= *c && *c <= 0x39)   goto parse_exponent_number;
    switch(*c) {
        CASE_NUMBER_TOKEN_END;
        default:                    goto error_number;
    }
parse_digit_leading_number:
    token.start = c;
    token.type.number = jsonlite_number_int | jsonlite_number_digit_leading;
parse_digits:
    if (++c == l)                   goto end_of_stream;
    if (0x30 <= *c && *c <= 0x39)   goto parse_digits;
    switch(*c) {
        CASE_NUMBER_TOKEN_END;
        case 0x2E:                  goto parse_fraction;
        case 0x45:                  goto parse_exponent;
        case 0x65:                  goto parse_exponent;
        default:                    goto error_number;
    }
number_parsed:
    token.end = c;
    state--;
    CALL_VALUE_CALLBACK(parser->callbacks, number_found, &token);
    goto select_state;

parse_object_key_or_end:
    switch (*c) {
        case 0x22:
            *state = state_object_colon;
            if (++state == last) goto error_depth;
            *state = state_object_key;
            goto parse_string;
        case 0x7D:
            state--;
            CALL_STATE_CALLBACK(parser->callbacks, object_end);
            goto structure_finished;
        default: goto error_exp_koe;
    }
parse_object_key_only:
    if (*c != 0x22) goto error_exp_key;
    *state = state_object_colon;
    *++state = state_object_key;

// String parsing
parse_string:
    token.type.string = jsonlite_string_ascii;
    token.start = c + 1;
next_char:
    if (++c == l)       goto end_of_stream;
    if (*c == 0x22)     goto string_parsed;
    if (*c == 0x5C)     goto escaped;
    if (*c > 0x7F)      goto utf8;
    if (*c < 0x20)      goto error_token;
    goto next_char;
escaped:
    token.type.string |= jsonlite_string_escape;
    if (++c == l)       goto end_of_stream;
    switch (*c) {
        case 0x22:      goto next_char;     // "    quotation mark  U+0022
        case 0x2F:      goto next_char;     // /    solidus         U+002F
        case 0x5C:      goto next_char;     // \    reverse solidus U+005C
        case 0x62:      goto next_char;     // b    backspace       U+0008
        case 0x66:      goto next_char;     // f    form feed       U+000C
        case 0x6E:      goto next_char;     // n    line feed       U+000A
        case 0x72:      goto next_char;     // r    carriage return U+000D
        case 0x74:      goto next_char;     // t    tab             U+0009
        case 0x75:      goto hex;           // uXXXX                U+XXXX
        default:        goto error_escape;
    }
hex:
    token.type.string |= jsonlite_string_unicode_escape;
    if (c++ + 4 >= l)       goto end_of_stream;
    HEX_CHAR_TO_INT(c, 1);  value = hex_value;
    HEX_CHAR_TO_INT(c, 1);  value = (uint32_t)(value << 4) | hex_value;
    HEX_CHAR_TO_INT(c, 1);  value = (uint32_t)(value << 4) | hex_value;
    HEX_CHAR_TO_INT(c, 0);  value = (uint32_t)(value << 4) | hex_value;

    if ((value & 0xFFFFu) >= 0xFFFEu)           token.type.string |= jsonlite_string_unicode_noncharacter;
    if (value >= 0xFDD0u && value <= 0xFDEFu)   token.type.string |= jsonlite_string_unicode_noncharacter;
    if (0xD800 > value || value > 0xDBFF)       goto next_char;

    // UTF-16 Surrogate
    utf32 = (value - 0xD800) << 10;
    if (c++ + 6 >= l)       goto end_of_stream;
    if (*c++ != 0x5C)       goto error_escape;
    if (*c++ != 0x75)       goto error_escape;
    HEX_CHAR_TO_INT(c, 1);  value = hex_value;
    HEX_CHAR_TO_INT(c, 1);  value = (uint32_t)(value << 4) | hex_value;
    HEX_CHAR_TO_INT(c, 1);  value = (uint32_t)(value << 4) | hex_value;
    HEX_CHAR_TO_INT(c, 0);  value = (uint32_t)(value << 4) | hex_value;

    if (value < 0xDC00 || value > 0xDFFF)       goto error_escape;
    utf32 += value - 0xDC00 + 0x10000;
    if ((utf32 & 0x0FFFFu) >= 0x0FFFEu)         token.type.string |= jsonlite_string_unicode_noncharacter;
    goto next_char;
utf8:
    token.type.string |= jsonlite_string_utf8;
    int res = jsonlite_clz((unsigned int)((*c) ^ 0xFF) << 0x19);
    utf32 = (*c & (0xFF >> (res + 1)));
    value = 0xAAAAAAAA; // == 1010...
    if (c + res >= l) goto end_of_stream;
    switch (res) {
        case 3: value = (value << 2) | (*++c >> 6); utf32 = (utf32 << 6) | (*c & 0x3F);
        case 2: value = (value << 2) | (*++c >> 6); utf32 = (utf32 << 6) | (*c & 0x3F);
        case 1: value = (value << 2) | (*++c >> 6); utf32 = (utf32 << 6) | (*c & 0x3F);
            if (value != 0xAAAAAAAA)                    goto error_utf8;
            if ((utf32 & 0xFFFFu) >= 0xFFFEu)           token.type.string |= jsonlite_string_unicode_noncharacter;
            if (utf32 >= 0xFDD0u && utf32 <= 0xFDEFu)   token.type.string |= jsonlite_string_unicode_noncharacter;
    }
    goto next_char;
string_parsed:
    token.end = c;
    parser->cursor = c + 1;
    if (*state-- == state_value) {
        CALL_VALUE_CALLBACK(parser->callbacks, string_found, &token);
    } else {
        CALL_VALUE_CALLBACK(parser->callbacks, key_found, &token);
    }
    goto skip_char_and_whitespaces;

// Primitive tokens
parse_true_token:
    if (c++ + 3 >= l)   goto end_of_stream;
    if (*c++ != 0x72)   goto error_token;
    if (*c++ != 0x75)   goto error_token;
    if (*c++ != 0x65)   goto error_token;
    state--;
    CALL_STATE_CALLBACK(parser->callbacks, true_found);
    goto select_state;
parse_false_token:
    if (c++ + 4 >= l)   goto end_of_stream;
    if (*c++ != 0x61)   goto error_token;
    if (*c++ != 0x6C)   goto error_token;
    if (*c++ != 0x73)   goto error_token;
    if (*c++ != 0x65)   goto error_token;
    state--;
    CALL_STATE_CALLBACK(parser->callbacks, false_found);
    goto select_state;
parse_null_token:
    if (c++ + 3 >= l)   goto end_of_stream;
    if (*c++ != 0x75)   goto error_token;
    if (*c++ != 0x6C)   goto error_token;
    if (*c++ != 0x6C)   goto error_token;
    state--;
    CALL_STATE_CALLBACK(parser->callbacks, null_found);
    goto select_state;

// Error states.
error_depth:        result = jsonlite_result_depth_limit;               goto end;
error_exp_ooa:      result = jsonlite_result_expected_object_or_array;  goto end;
error_exp_value:    result = jsonlite_result_expected_value;            goto end;
error_exp_koe:      result = jsonlite_result_expected_key_or_end;       goto end;
error_exp_key:      result = jsonlite_result_expected_key;              goto end;
error_exp_colon:    result = jsonlite_result_expected_colon;            goto end;
error_exp_coe:      result = jsonlite_result_expected_comma_or_end;     goto end;
error_escape:       result = jsonlite_result_invalid_escape;            goto end;
error_number:       result = jsonlite_result_invalid_number;            goto end;
error_token:        result = jsonlite_result_invalid_token;             goto end;
error_utf8:         result = jsonlite_result_invalid_utf8;              goto end;

// End of stream states.
end_of_stream_whitespaces:
    token_start = l;
end_of_stream:
    result = jsonlite_result_end_of_stream;
end:
    parser->result = result;
    parser->current = state;
    parser->control = NULL;
    parser->cursor = c;
    parser->callbacks.parse_finished(&parser->callbacks.context);
    if (result != jsonlite_result_end_of_stream) {
        return;
    }

    res = jsonlite_buffer_set_mem(parser->rest_buffer, token_start, parser->limit - token_start);
    if (res < 0) {
        parser->result = jsonlite_result_out_of_memory;
    } else {
        parser->buffer = jsonlite_buffer_data(parser->rest_buffer);
        parser->limit = parser->buffer + jsonlite_buffer_size(parser->rest_buffer);
    }
}
//
//  Copyright 2012-2014, Andrii Mamchur
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License

#ifndef JSONLITE_AMALGAMATED
#include "../include/jsonlite_stream.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int jsonlite_stream_write(jsonlite_stream stream, const void *data, size_t length) {
    return stream->write(stream, data, length);
}

#define CAST_TO_MEM_STREAM(S)   (jsonlite_mem_stream *)((uint8_t *)(S) + sizeof(jsonlite_stream_struct))
#define SIZE_OF_MEM_STREAM()    (sizeof(jsonlite_stream_struct) + sizeof(jsonlite_mem_stream))

static int jsonlite_mem_stream_write(jsonlite_stream stream, const void *data, size_t length) {
    jsonlite_mem_stream *mem_stream = CAST_TO_MEM_STREAM(stream);
    size_t write_limit = mem_stream->limit - mem_stream->cursor;
    if (write_limit >= length) {
        memcpy(mem_stream->cursor, data, length);       // LCOV_EXCL_LINE
        mem_stream->cursor += length;
    } else {
        memcpy(mem_stream->cursor, data, write_limit);  // LCOV_EXCL_LINE
        mem_stream->cursor += write_limit;

        size_t size = sizeof(jsonlite_mem_stream_block) + mem_stream->block_size;
        jsonlite_mem_stream_block *block = malloc(size);
        block->data = (uint8_t *)block + sizeof(jsonlite_mem_stream_block);
        block->next = NULL;

        mem_stream->current->next = block;
        mem_stream->current = block;
        mem_stream->cursor = block->data;
        mem_stream->limit = block->data + mem_stream->block_size;

        jsonlite_mem_stream_write(stream, (char *)data + write_limit, length - write_limit);
    }

    return (int)length;
}

void jsonlite_mem_stream_free(jsonlite_stream stream) {
    jsonlite_mem_stream *mem_stream = CAST_TO_MEM_STREAM(stream);
    jsonlite_mem_stream_block *block = mem_stream->first;
    void *prev;
    for (; block != NULL;) {
        prev = block;
        block = block->next;
        free(prev);
    }

    free((void *)stream);
}

jsonlite_stream jsonlite_mem_stream_alloc(size_t block_size) {
    size_t size = SIZE_OF_MEM_STREAM();
    struct jsonlite_stream_struct *stream = malloc(size);
    stream->write = jsonlite_mem_stream_write;

    jsonlite_mem_stream_block *first = malloc(sizeof(jsonlite_mem_stream_block) + block_size);
    first->data = (uint8_t *)first + sizeof(jsonlite_mem_stream_block);
    first->next = NULL;

    jsonlite_mem_stream *mem_stream = CAST_TO_MEM_STREAM(stream);
    mem_stream->block_size = block_size;
    mem_stream->cursor = first->data;
    mem_stream->limit = first->data + block_size;
    mem_stream->current = first;
    mem_stream->first = first;
    return stream;
}

size_t jsonlite_mem_stream_data(jsonlite_stream stream, uint8_t **data, size_t extra_bytes) {
    jsonlite_mem_stream *mem_stream = CAST_TO_MEM_STREAM(stream);
    jsonlite_mem_stream_block *block = NULL;
    uint8_t *buff = NULL;
    size_t size = 0;

    for (block = mem_stream->first; block != NULL; block = block->next) {
        if (block->next != NULL) {
            size += mem_stream->block_size;
        } else {
            size += mem_stream->cursor - block->data;
        }
    }

    if (size == 0) {
        *data = NULL;
    } else {
        *data = (uint8_t *)malloc(size + extra_bytes);
        buff = *data;
        for (block = mem_stream->first; block != NULL; block = block->next) {
            if (block->next != NULL) {
                memcpy(buff, block->data, mem_stream->block_size); // LCOV_EXCL_LINE
                buff += mem_stream->block_size;
            } else {
                memcpy(buff, block->data, mem_stream->cursor - block->data); // LCOV_EXCL_LINE
            }
        }
    }

    return size;
}

static int jsonlite_static_mem_stream_write(jsonlite_stream stream, const void *data, size_t length) {
    jsonlite_static_mem_stream *mem_stream = (jsonlite_static_mem_stream *)((uint8_t *)stream + sizeof(jsonlite_stream_struct));
    size_t write_limit = mem_stream->size - mem_stream->written;
    if (mem_stream->enabled && write_limit >= length) {
        memcpy(mem_stream->buffer + mem_stream->written, data, length); // LCOV_EXCL_LINE
        mem_stream->written += length;
    } else {
        mem_stream->enabled = 0;
        return 0;
    }

    return (int)length;
}

jsonlite_stream jsonlite_static_mem_stream_init(void *buffer, size_t size) {
    int extra_size = (int)size - sizeof(jsonlite_stream_struct) - sizeof(jsonlite_static_mem_stream);
    if (extra_size <= 0) {
        return NULL;
    }

    struct jsonlite_stream_struct *stream = (struct jsonlite_stream_struct *)buffer;
    stream->write = jsonlite_static_mem_stream_write;

    jsonlite_static_mem_stream *mem_stream = (jsonlite_static_mem_stream *)((uint8_t *)stream + sizeof(jsonlite_stream_struct));
    mem_stream->buffer = (uint8_t *)mem_stream + sizeof(jsonlite_static_mem_stream);
    mem_stream->size = (size_t)extra_size;
    mem_stream->written = 0;
    mem_stream->enabled = 1;
    return stream;
}

size_t jsonlite_static_mem_stream_written_bytes(jsonlite_stream stream) {
    jsonlite_static_mem_stream *mem_stream = (jsonlite_static_mem_stream *)((uint8_t *)stream + sizeof(jsonlite_stream_struct));
    return mem_stream->written;
}

const void * jsonlite_static_mem_stream_data(jsonlite_stream stream) {
    jsonlite_static_mem_stream *mem_stream = (jsonlite_static_mem_stream *)((uint8_t *)stream + sizeof(jsonlite_stream_struct));
    return mem_stream->buffer;
}

#define CAST_TO_FILE_STREAM(S)   (jsonlite_file_stream *)((uint8_t *)(S) + sizeof(jsonlite_stream_struct))
#define SIZE_OF_FILE_STREAM()    (sizeof(jsonlite_stream_struct) + sizeof(jsonlite_file_stream))

typedef struct jsonlite_file_stream {
    FILE *file;
} jsonlite_file_stream;

static int jsonlite_file_stream_write(jsonlite_stream stream, const void *data, size_t length) {
    jsonlite_file_stream *file_stream = CAST_TO_FILE_STREAM(stream);
    return (int)fwrite(data, 1, length, file_stream->file);
}

jsonlite_stream jsonlite_file_stream_alloc(FILE *file) {
    size_t size = SIZE_OF_FILE_STREAM();
    struct jsonlite_stream_struct *stream = malloc(size);
    stream->write = jsonlite_file_stream_write;

    jsonlite_file_stream *file_stream = CAST_TO_FILE_STREAM(stream);
    file_stream->file = file;
    return stream;
}

void jsonlite_file_stream_free(jsonlite_stream stream) {
    free((void *)stream);
}

static int jsonlite_null_stream_write(jsonlite_stream stream, const void *data, size_t length) {
    return (int)length;
}

static int jsonlite_stdout_stream_write(jsonlite_stream stream, const void *data, size_t length) {
    return (int)fwrite(data, 1, length, stdout);
}

static struct jsonlite_stream_struct jsonlite_stdout_stream_struct = {jsonlite_stdout_stream_write};
static struct jsonlite_stream_struct jsonlite_null_stream_struct = {jsonlite_null_stream_write};

jsonlite_stream jsonlite_stdout_stream = &jsonlite_stdout_stream_struct;
jsonlite_stream jsonlite_null_stream = &jsonlite_null_stream_struct;
//
//  Copyright 2012-2014, Andrii Mamchur
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License

#ifndef JSONLITE_AMALGAMATED
#include "../include/jsonlite_token.h"
#endif

#include <stdlib.h>

#ifdef _MSC_VER
#include <intrin.h>

static uint32_t __inline jsonlite_clz( uint32_t x ) {
   unsigned long r = 0;
   _BitScanForward(&r, x);
   return r;
}

#else

#define jsonlite_clz(x) __builtin_clz((x))

#endif

static uint8_t jsonlite_hex_char_to_uint8(uint8_t c) {
    if (c >= 'a') {
        return c - 'a' + 10;
    }

    if (c >= 'A') {
        return c - 'A' + 10;
    }

    return c - '0';
}

static int unicode_char_to_utf16(uint32_t ch, uint16_t *utf16) {
    uint32_t v = ch - 0x10000;
    uint32_t vh = v >> 10;
    uint32_t vl = v & 0x3FF;
  if (ch <= 0xFFFF) {
        *utf16 = (uint16_t)ch;
        return 1;
    }

    *utf16++ = (uint16_t)(0xD800 + vh);
    *utf16 = (uint16_t)(0xDC00 + vl);
    return 2;
}

size_t jsonlite_token_size_of_uft8(jsonlite_token *ts) {
    return (size_t)(ts->end - ts->start + 1);
}

size_t jsonlite_token_to_uft8(jsonlite_token *ts, uint8_t *buffer) {
    const uint8_t *p = ts->start;
    const uint8_t *l = ts->end;
    uint32_t value, utf32;
    uint8_t *c = buffer;
    int res;
step:
    if (p == l)         goto done;
    if (*p == '\\')     goto escaped;
    if (*p >= 0x80)     goto utf8;
    *c++ = *p++;
    goto step;
escaped:
    switch (*++p) {
        case 34:    *c++ = '"';     p++; goto step;
        case 47:    *c++ = '/';     p++; goto step;
        case 92:    *c++ = '\\';    p++; goto step;
        case 98:    *c++ = '\b';    p++; goto step;
        case 102:   *c++ = '\f';    p++; goto step;
        case 110:   *c++ = '\n';    p++; goto step;
        case 114:   *c++ = '\r';    p++; goto step;
        case 116:   *c++ = '\t';    p++; goto step;
  }

    // UTF-16
    p++;
    utf32 = jsonlite_hex_char_to_uint8(*p++);
    utf32 = (uint32_t)(utf32 << 4) | jsonlite_hex_char_to_uint8(*p++);
    utf32 = (uint32_t)(utf32 << 4) | jsonlite_hex_char_to_uint8(*p++);
    utf32 = (uint32_t)(utf32 << 4) | jsonlite_hex_char_to_uint8(*p++);
    if (0xD800 > utf32 || utf32 > 0xDBFF) goto encode;

    // UTF-16 Surrogate
    p += 2;
    utf32 = (utf32 - 0xD800) << 10;
    value = jsonlite_hex_char_to_uint8(*p++);
    value = (uint32_t)(value << 4) | jsonlite_hex_char_to_uint8(*p++);
    value = (uint32_t)(value << 4) | jsonlite_hex_char_to_uint8(*p++);
    value = (uint32_t)(value << 4) | jsonlite_hex_char_to_uint8(*p++);
    utf32 += value - 0xDC00 + 0x10000;
encode:
    if (utf32 < 0x80) {
        *c++ = (uint8_t)utf32;
    } else if (utf32 < 0x0800) {
        c[1] = (uint8_t)(utf32 & 0x3F) | 0x80;
        utf32 = utf32 >> 6;
        c[0] = (uint8_t)utf32 | 0xC0;
        c += 2;
    } else if (utf32 < 0x10000) {
        c[2] = (uint8_t)(utf32 & 0x3F) | 0x80;
        utf32 = utf32 >> 6;
        c[1] = (uint8_t)(utf32 & 0x3F) | 0x80;
        utf32 = utf32 >> 6;
        c[0] = (uint8_t)utf32 | 0xE0;
        c += 3;
    } else {
        c[3] = (uint8_t)(utf32 & 0x3F) | 0x80;
        utf32 = utf32 >> 6;
        c[2] = (uint8_t)(utf32 & 0x3F) | 0x80;
        utf32 = utf32 >> 6;
        c[1] = (uint8_t)(utf32 & 0x3F) | 0x80;
        utf32 = utf32 >> 6;
        c[0] = (uint8_t)utf32 | 0xF0;
        c += 4;
    }
    goto step;
utf8:
    res = jsonlite_clz(((*p) ^ 0xFF) << 0x19);
    *c++ = *p++;
    switch (res) {
        case 3: *c++ = *p++;
        case 2: *c++ = *p++;
        case 1: *c++ = *p++;
    }
    goto step;
done:
    *c = 0;
    return c - buffer;
}

size_t jsonlite_token_size_of_uft16(jsonlite_token *ts) {
    return (ts->end - ts->start + 1) * sizeof(uint16_t);
}

size_t jsonlite_token_to_uft16(jsonlite_token *ts, uint16_t *buffer) {
    const uint8_t *p = ts->start;
    const uint8_t *l = ts->end;
    uint16_t utf16;
    uint16_t *c = buffer;
    int res;
step:
    if (p == l)         goto done;
    if (*p == '\\')     goto escaped;
    if (*p >= 0x80)     goto utf8;
    *c++ = *p++;
    goto step;
escaped:
    switch (*++p) {
        case 34:    *c++ = '"';     p++; goto step;
        case 47:    *c++ = '/';     p++; goto step;
        case 92:    *c++ = '\\';    p++; goto step;
        case 98:    *c++ = '\b';    p++; goto step;
        case 102:   *c++ = '\f';    p++; goto step;
        case 110:   *c++ = '\n';    p++; goto step;
        case 114:   *c++ = '\r';    p++; goto step;
        case 116:   *c++ = '\t';    p++; goto step;
  }

    // UTF-16
    p++;
    utf16 = jsonlite_hex_char_to_uint8(*p++);
    utf16 = (uint16_t)(utf16 << 4) | jsonlite_hex_char_to_uint8(*p++);
    utf16 = (uint16_t)(utf16 << 4) | jsonlite_hex_char_to_uint8(*p++);
    utf16 = (uint16_t)(utf16 << 4) | jsonlite_hex_char_to_uint8(*p++);
    *c++ = utf16;
    if (0xD800 > utf16 || utf16 > 0xDBFF) goto step;

    // UTF-16 Surrogate
    p += 2;
    utf16 = jsonlite_hex_char_to_uint8(*p++);
    utf16 = (uint16_t)(utf16 << 4) | jsonlite_hex_char_to_uint8(*p++);
    utf16 = (uint16_t)(utf16 << 4) | jsonlite_hex_char_to_uint8(*p++);
    utf16 = (uint16_t)(utf16 << 4) | jsonlite_hex_char_to_uint8(*p++);
    *c++ = utf16;
    goto step;
utf8:
    res = jsonlite_clz(((*p) ^ 0xFF) << 0x19);
    uint32_t code = (*p & (0xFF >> (res + 1)));
    switch (res) {
        case 3: code = (code << 6) | (*++p & 0x3F);
        case 2: code = (code << 6) | (*++p & 0x3F);
        case 1: code = (code << 6) | (*++p & 0x3F);
        case 0: ++p;
    }

    c += unicode_char_to_utf16(code, c);
    goto step;
done:
    *c = 0;
    return (c - buffer) * sizeof(uint16_t);
}

size_t jsonlite_token_size_of_base64_binary(jsonlite_token *ts) {
    return (((ts->end - ts->start) * 3) / 4 + 3) & ~3;
}

size_t jsonlite_token_base64_to_binary(jsonlite_token *ts, void *buffer) {
    size_t length = 0;
    const uint8_t *p = ts->start;
    const uint8_t *l = ts->end;
    uint8_t *c = buffer;
    size_t bytes, i;
next:
    bytes = 0;
    i = 0;
    do {
        if (p == l) goto error;
        i++;
        bytes <<= 6;
        if (0x41 <= *p && *p <= 0x5A) { bytes |= *p++ - 0x41; continue; }
        if (0x61 <= *p && *p <= 0x7A) { bytes |= *p++ - 0x47; continue; }
        if (0x30 <= *p && *p <= 0x39) { bytes |= *p++ + 0x04; continue; }
        if (*p == 0x2B) { bytes |= 0x3E; p++; continue; }
        if (*p == 0x2F) { bytes |= 0x3F; p++; continue; }
        if (*p == '=' && length > 0) {
            switch (l - p) {
                case 1:
                    *c++ = (uint8_t)((bytes >> 16) & 0x000000FF);
                    *c = (uint8_t)((bytes >> 8) & 0x000000FF);
                    return length + 2;
                case 2:
                    *c = (uint8_t)((bytes >> 10) & 0x000000FF);
                    return length + 1;
            }
        }
        if (*p == 0x5C && *++p == 0x2F) { bytes |= 0x3F; p++; continue; }
        goto error;
    } while (i < 4);

    *c++ = (uint8_t)((bytes >> 16)  & 0x000000FF);
    *c++ = (uint8_t)((bytes >> 8)   & 0x000000FF);
    *c++ = (uint8_t)((bytes)        & 0x000000FF);
    length += 3;

    if (p == l) goto done;
    goto next;
error:
    length = 0;
done:
    return length;
}

long jsonlite_token_to_long(jsonlite_token *token) {
    long res = 0;
    int negative = (token->type.number & jsonlite_number_negative) == jsonlite_number_negative;
    ptrdiff_t length = token->end - token->start - negative;
    const uint8_t *c = token->start + negative;
    switch (length & 3) {
        for (; length > 0; length -= 4) {
            case 0: res = res * 10 + *c++ - '0';
            case 3: res = res * 10 + *c++ - '0';
            case 2: res = res * 10 + *c++ - '0';
            case 1: res = res * 10 + *c++ - '0';
        }
    }

    return negative ? -res : res;
}

long long jsonlite_token_to_long_long(jsonlite_token *token) {
    long long res = 0;
    int negative = (token->type.number & jsonlite_number_negative) == jsonlite_number_negative;
    ptrdiff_t length = token->end - token->start - negative;
    const uint8_t *c = token->start + negative;
    switch (length & 7) {
        for (; length > 0; length -= 8) {
            case 0: res = res * 10 + *c++ - '0';
            case 7: res = res * 10 + *c++ - '0';
            case 6: res = res * 10 + *c++ - '0';
            case 5: res = res * 10 + *c++ - '0';
            case 4: res = res * 10 + *c++ - '0';
            case 3: res = res * 10 + *c++ - '0';
            case 2: res = res * 10 + *c++ - '0';
            case 1: res = res * 10 + *c++ - '0';
        }
    }

    return negative ? -res : res;
}
//
//  Copyright 2012-2014, Andrii Mamchur
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License

#ifndef JSONLITE_AMALGAMATED
#include "../include/jsonlite_token_pool.h"
#endif

#include <stdlib.h>
#include <string.h>

static void jsonlite_extend_capacity(jsonlite_token_pool pool, ptrdiff_t index);
static uint32_t jsonlite_hash(const uint8_t *data, size_t len);
static jsonlite_token_bucket terminate_bucket = {NULL, NULL, 0, 0, NULL};

size_t jsonlite_token_pool_init_memory(void *mem, size_t size, jsonlite_token_pool* pools) {
    int i, j;
    jsonlite_token_pool p = (jsonlite_token_pool)mem;
    size_t count = size / sizeof(jsonlite_token_pool_struct);
    for (i = 0; i < count; i++, p++, pools++) {
        *pools = p;
        p->content_pool = NULL;
        p->content_pool_size = 0;

        for (j = 0; j < JSONLITE_TOKEN_POOL_FRONT; j++) {
            p->blocks[j].buckets = &terminate_bucket;
            p->blocks[j].capacity = 0;
        }
    }

    return count;
}

void jsonlite_token_pool_copy_tokens(jsonlite_token_pool pool) {
    jsonlite_token_bucket *bucket;
    size_t size = 0;
    int i;
    for (i = 0; i < JSONLITE_TOKEN_POOL_FRONT; i++) {
        bucket = pool->blocks[i].buckets;
        while (bucket->start != NULL) {
            size += bucket->end - bucket->start;
            bucket++;
        }
    }

    if (size == pool->content_pool_size) {
        return;
    }

  uint8_t *buffer = (uint8_t *)malloc(size);
    uint8_t *p = buffer;
    for (i = 0; i < JSONLITE_TOKEN_POOL_FRONT; i++) {
        bucket = pool->blocks[i].buckets;
        while (bucket->start != NULL) {
            size_t length = bucket->end - bucket->start;
            memcpy(p, bucket->start, length); // LCOV_EXCL_LINE
            bucket->start = p;
            bucket->end = p + length;
            p += length;
            bucket++;
        }
    }

    free(pool->content_pool);
    pool->content_pool = buffer;
    pool->content_pool_size = size;
}

void jsonlite_token_pool_cleanup(jsonlite_token_pool* pools, size_t count, jsonlite_token_pool_release_value_fn release) {
    int i, j;
    jsonlite_token_pool pool = *pools;
    for (i = 0; i < count; i++, pool++) {
        for (j = 0; j < JSONLITE_TOKEN_POOL_FRONT; j++) {
            jsonlite_token_bucket *bucket = pool->blocks[j].buckets;
            if (bucket->start == NULL) {
                continue;
            }

            if (release != NULL) {
                for (; bucket->start != NULL; bucket++) {
                    release((void *)bucket->value);
                }
            }

            free(pool->blocks[j].buckets);
            pool->blocks[j].buckets = &terminate_bucket;
            pool->blocks[j].capacity = 0;
        }

        free(pool->content_pool);
        pool->content_pool = NULL;
        pool->content_pool_size = 0;
    }
}

jsonlite_token_bucket* jsonlite_token_pool_get_bucket(jsonlite_token_pool pool, jsonlite_token *token) {
    ptrdiff_t length = token->end - token->start;
    ptrdiff_t hash = jsonlite_hash(token->start, (size_t)length);
    ptrdiff_t index = hash & JSONLITE_TOKEN_POOL_FRONT_MASK;
    size_t count = 0;
    jsonlite_token_bucket *bucket = pool->blocks[index].buckets;
    for (; bucket->start != NULL; count++, bucket++) {
        if (bucket->hash != hash) {
            continue;
        }

        if (length != bucket->end - bucket->start) {
            continue;
        }

        if (memcmp(token->start, bucket->start, (size_t)length) == 0) {
            return bucket;
        }
    }

    size_t capacity = pool->blocks[index].capacity;
    if (count + 1 >= capacity) {
        jsonlite_extend_capacity(pool, index);
    }

    bucket = pool->blocks[index].buckets + count;
    bucket->hash = hash;
    bucket->start = token->start;
    bucket->end = token->end;
    bucket->value = NULL;
    bucket[1].start = NULL;
    return bucket;
}

static void jsonlite_extend_capacity(jsonlite_token_pool pool, ptrdiff_t index) {
    size_t capacity = pool->blocks[index].capacity;
    if (capacity == 0) {
        capacity = 0x10;
    }

    size_t size = capacity * sizeof(jsonlite_token_bucket);
    jsonlite_token_bucket *buckets = pool->blocks[index].buckets;
  jsonlite_token_bucket *extended = (jsonlite_token_bucket *)malloc(2 * size);

    if (buckets->start != NULL) {
        memcpy(extended, buckets, size); // LCOV_EXCL_LINE
        free(buckets);
    }

    pool->blocks[index].buckets = extended;
    pool->blocks[index].capacity = 2 * capacity;
}

// Used MurmurHash2 function by Austin Appleby
// http://code.google.com/p/smhasher/ revision 147

//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

static uint32_t MurmurHash2(const void * key, size_t len)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value

    uint32_t h = (uint32_t)len;

    // Mix 4 bytes at a time into the hash

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        uint32_t k = *(uint32_t*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch(len)
    {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
            h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

//-----------------------------------------------------------------------------

static uint32_t jsonlite_hash(const uint8_t *data, size_t len) {
    return MurmurHash2(data, len);
}