/* xchg.c
 * Copyright (c) 2019 Alex Forster
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <string.h>
#include <stdatomic.h>

#include "xchg.h"

#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)

static size_t flp2(size_t x)
{
    size_t y = (size_t)1 << (size_t)63;
    while(likely(y > x && y > 2))
    {
        y = y >> 1u;
    }
    return y;
}

static uint8_t lsz_from_sz_list(size_t sz_list)
{
    return (sz_list == 0) ? 0 : (sz_list <= 0xFF) ? 1 : (sz_list <= 0xFFFF) ? 2 : 3;
}

static uint8_t lsz_to_nr_bytes(uint8_t lsz)
{
    return (lsz == 0) ? 0 : (lsz == 1) ? 1 : (lsz == 2) ? 2 : 8;
}

struct __attribute__((packed, aligned(1))) xchg_tag
{
    enum xchg_type type : 4;
    uint8_t lsz : 2;
    bool list : 1;
    bool null : 1;
};

struct xchg_value
{
    enum xchg_type type;
    bool null;
    bool list;
    size_t sz_list;
    char *data;
    size_t sz_data;
};

bool xchg_message_init(struct xchg_message *message, char *data, size_t sz_data)
{
    if(unlikely(message == NULL || data == NULL || sz_data == 0))
    {
        return false;
    }

    message->data = data;
    message->length = sz_data;
    message->position = 0;
    message->error = NULL;

    return true;
}

bool xchg_message_reset(struct xchg_message *message)
{
    if(unlikely(message == NULL))
    {
        return false;
    }

    message->position = 0;
    message->error = NULL;

    message->error = NULL;
    return true;
}

bool xchg_message_position(struct xchg_message *message, size_t *position)
{
    if(unlikely(message == NULL || position == NULL))
    {
        return false;
    }

    *position = message->position;

    message->error = NULL;
    return true;
}

bool xchg_message_seek(struct xchg_message *message, size_t position)
{
    if(unlikely(message == NULL))
    {
        return false;
    }

    if(message->length <= position)
    {
        message->error = "attempt to seek outside message bounds";
        return false;
    }

    message->position = position;
    message->error = NULL;
    return true;
}

bool xchg_message_peek(struct xchg_message *message, enum xchg_type *type, bool *null, bool *list, uint64_t *sz_list)
{
    if(unlikely(message == NULL))
    {
        return false;
    }

    size_t position = message->position;

    if(unlikely((position + sizeof(struct xchg_tag)) > message->length))
    {
        message->error = "the message has no more data left to read";
        return false;
    }

    struct xchg_tag tag;
    memcpy(&tag, &((uint8_t *)message->data)[position], sizeof(struct xchg_tag));
    position += sizeof(struct xchg_tag);

    size_t _sz_list = 0;

    if(!tag.null && tag.list && tag.lsz > 0)
    {
        uint8_t nr_bytes = lsz_to_nr_bytes(tag.lsz);

        if(unlikely((position + nr_bytes) > message->length))
        {
            message->error = "the message is not large enough to read the expected amount of data";
            return false;
        }

        memcpy(&_sz_list, &((uint8_t *)message->data)[position], nr_bytes);
        position += nr_bytes;
    }

    size_t sz_data = 0;

    switch(tag.type)
    {
    case xchg_type_bool:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(bool) : sizeof(bool);
        break;
    case xchg_type_int8:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(int8_t) : sizeof(int8_t);
        break;
    case xchg_type_uint8:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(uint8_t) : sizeof(uint8_t);
        break;
    case xchg_type_int16:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(int16_t) : sizeof(int16_t);
        break;
    case xchg_type_uint16:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(uint16_t) : sizeof(uint16_t);
        break;
    case xchg_type_int32:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(int32_t) : sizeof(int32_t);
        break;
    case xchg_type_uint32:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(uint32_t) : sizeof(uint32_t);
        break;
    case xchg_type_int64:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(int64_t) : sizeof(int64_t);
        break;
    case xchg_type_uint64:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(uint64_t) : sizeof(uint64_t);
        break;
    case xchg_type_float32:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(float_t) : sizeof(float_t);
        break;
    case xchg_type_float64:
        sz_data = tag.null ? 0 : tag.list ? _sz_list * sizeof(double_t) : sizeof(double_t);
        break;
    default:
        message->error = "value type should be one of xchg_type_t";
        return false;
    }

    if(!tag.null)
    {
        if(unlikely((position + sz_data) > message->length))
        {
            message->error = "the message is not large enough to read the expected amount of data";
            return false;
        }
    }

    if(sz_list != NULL)
    {
        *sz_list = _sz_list;
    }

    if(type != NULL)
    {
        *type = tag.type;
    }

    if(null != NULL)
    {
        *null = tag.null;
    }

    if(list != NULL)
    {
        *list = tag.list;
    }

    message->error = NULL;
    return true;
}

const char *xchg_message_strerror(const struct xchg_message *message)
{
    if(unlikely(message == NULL))
    {
        return false;
    }

    return message->error;
}

bool xchg_message_read(struct xchg_message *message, struct xchg_value *value)
{
    if(unlikely(message == NULL || value == NULL))
    {
        return false;
    }

    size_t position = message->position;

    if(unlikely((position + sizeof(struct xchg_tag)) > message->length))
    {
        message->error = "the message has no more data left to read";
        return false;
    }

    struct xchg_tag tag;
    memcpy(&tag, &((uint8_t *)message->data)[position], sizeof(struct xchg_tag));
    position += sizeof(struct xchg_tag);

    size_t sz_list = 0;

    if(!tag.null && tag.list && tag.lsz > 0)
    {
        uint8_t nr_bytes = lsz_to_nr_bytes(tag.lsz);

        if(unlikely((position + nr_bytes) > message->length))
        {
            message->error = "the message is not large enough to read the expected amount of data";
            return false;
        }

        memcpy(&sz_list, &((uint8_t *)message->data)[position], nr_bytes);
        position += nr_bytes;
    }

    size_t sz_data = 0;

    switch(tag.type)
    {
    case xchg_type_bool:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(bool) : sizeof(bool);
        break;
    case xchg_type_int8:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(int8_t) : sizeof(int8_t);
        break;
    case xchg_type_uint8:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(uint8_t) : sizeof(uint8_t);
        break;
    case xchg_type_int16:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(int16_t) : sizeof(int16_t);
        break;
    case xchg_type_uint16:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(uint16_t) : sizeof(uint16_t);
        break;
    case xchg_type_int32:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(int32_t) : sizeof(int32_t);
        break;
    case xchg_type_uint32:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(uint32_t) : sizeof(uint32_t);
        break;
    case xchg_type_int64:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(int64_t) : sizeof(int64_t);
        break;
    case xchg_type_uint64:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(uint64_t) : sizeof(uint64_t);
        break;
    case xchg_type_float32:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(float_t) : sizeof(float_t);
        break;
    case xchg_type_float64:
        sz_data = tag.null ? 0 : tag.list ? sz_list * sizeof(double_t) : sizeof(double_t);
        break;
    default:
        message->error = "value type should be one of xchg_type_t";
        return false;
    }

    char *data = NULL;

    if(sz_data > 0)
    {
        if(unlikely((position + sz_data) > message->length))
        {
            message->error = "the message is not large enough to read the expected amount of data";
            return false;
        }

        data = &((char *)message->data)[position];
        position += sz_data;
    }

    value->type = tag.type;
    value->null = tag.null;
    value->list = tag.list;
    value->sz_list = sz_list;
    value->data = data;
    value->sz_data = sz_data;

    message->position = position;
    message->error = NULL;
    return true;
}

#define XCHG_MESSAGE_READ_HELPERS(xchg_type, c_type)                                                                 \
    bool xchg_message_read_##xchg_type(struct xchg_message *message, c_type *value)                                  \
    {                                                                                                                \
        if(unlikely(message == NULL || value == NULL))                                                               \
        {                                                                                                            \
            return false;                                                                                            \
        }                                                                                                            \
                                                                                                                     \
        if(unlikely((message->position + sizeof(struct xchg_tag)) > message->length))                                \
        {                                                                                                            \
            message->error = "the message has no more data left to read";                                            \
            return false;                                                                                            \
        }                                                                                                            \
                                                                                                                     \
        struct xchg_tag tag;                                                                                         \
        memcpy(&tag, &((uint8_t *)message->data)[message->position], sizeof(struct xchg_tag));                       \
                                                                                                                     \
        struct xchg_value v;                                                                                         \
        if(tag.list || tag.null || tag.type != xchg_type_##xchg_type || !xchg_message_read(message, &v))             \
        {                                                                                                            \
            return false;                                                                                            \
        }                                                                                                            \
                                                                                                                     \
        *value = *(c_type *)v.data;                                                                                  \
        return true;                                                                                                 \
    }                                                                                                                \
                                                                                                                     \
    bool xchg_message_read_##xchg_type##_list(struct xchg_message *message, c_type const *list[], uint64_t *sz_list) \
    {                                                                                                                \
        if(unlikely(message == NULL || list == NULL || sz_list == NULL))                                             \
        {                                                                                                            \
            return false;                                                                                            \
        }                                                                                                            \
                                                                                                                     \
        if(unlikely((message->position + sizeof(struct xchg_tag)) > message->length))                                \
        {                                                                                                            \
            message->error = "the message has no more data left to read";                                            \
            return false;                                                                                            \
        }                                                                                                            \
                                                                                                                     \
        struct xchg_tag tag;                                                                                         \
        memcpy(&tag, &((uint8_t *)message->data)[message->position], sizeof(struct xchg_tag));                       \
                                                                                                                     \
        struct xchg_value v;                                                                                         \
        if(!tag.list || tag.null || tag.type != xchg_type_##xchg_type || !xchg_message_read(message, &v))            \
        {                                                                                                            \
            return false;                                                                                            \
        }                                                                                                            \
                                                                                                                     \
        *sz_list = v.sz_list;                                                                                        \
        *list = (const c_type *)v.data;                                                                              \
        return true;                                                                                                 \
    }

bool xchg_message_read_null(struct xchg_message *message, enum xchg_type *type)
{
    if(unlikely(message == NULL || type == NULL))
    {
        return false;
    }

    if(unlikely((message->position + sizeof(struct xchg_tag)) > message->length))
    {
        message->error = "the message has no more data left to read";
        return false;
    }

    struct xchg_tag tag;
    memcpy(&tag, &((uint8_t *)message->data)[message->position], sizeof(struct xchg_tag));

    struct xchg_value value;
    if(tag.list || !tag.null || !xchg_message_read(message, &value))
    {
        return false;
    }

    *type = value.type;
    return true;
}

bool xchg_message_read_null_list(struct xchg_message *message, enum xchg_type *type)
{
    if(unlikely(message == NULL || type == NULL))
    {
        return false;
    }

    if(unlikely((message->position + sizeof(struct xchg_tag)) > message->length))
    {
        message->error = "the message has no more data left to read";
        return false;
    }

    struct xchg_tag tag;
    memcpy(&tag, &((uint8_t *)message->data)[message->position], sizeof(struct xchg_tag));

    struct xchg_value v;
    if(!tag.list || !tag.null || !xchg_message_read(message, &v))
    {
        return false;
    }

    *type = v.type;
    return true;
}

XCHG_MESSAGE_READ_HELPERS(bool, bool)
XCHG_MESSAGE_READ_HELPERS(int8, int8_t)
XCHG_MESSAGE_READ_HELPERS(uint8, uint8_t)
XCHG_MESSAGE_READ_HELPERS(int16, int16_t)
XCHG_MESSAGE_READ_HELPERS(uint16, uint16_t)
XCHG_MESSAGE_READ_HELPERS(int32, int32_t)
XCHG_MESSAGE_READ_HELPERS(uint32, uint32_t)
XCHG_MESSAGE_READ_HELPERS(int64, int64_t)
XCHG_MESSAGE_READ_HELPERS(uint64, uint64_t)
XCHG_MESSAGE_READ_HELPERS(float32, float_t)
XCHG_MESSAGE_READ_HELPERS(float64, double_t)

#undef XCHG_MESSAGE_READ_HELPERS

bool xchg_message_write(struct xchg_message *message, const struct xchg_value *value)
{
    if(unlikely(message == NULL || value == NULL))
    {
        return false;
    }

    if(unlikely(value->null && (value->data != NULL || value->sz_data > 0 || value->sz_list > 0)))
    {
        message->error = "value is null, so data/sz_data/sz_list should be unset";
        return false;
    }

    if(unlikely(!value->null && (value->data == NULL || value->sz_data == 0) && !(value->list && value->sz_list == 0)))
    {
        message->error = "value is not null and not an empty list, so data/sz_data should be set";
        return false;
    }

    if(unlikely(value->list && value->sz_list == 0 && value->sz_data > 0))
    {
        message->error = "value is an empty list, so sz_data should be unset";
        return false;
    }

    if(unlikely(value->list && value->sz_list > 0 && (value->data == NULL || value->sz_data == 0)))
    {
        message->error = "value is a sized list, so data/sz_data should be set";
        return false;
    }

    size_t sz_data = 0;

    if(!value->null)
    {
        switch(value->type)
        {
        case xchg_type_bool:
            sz_data = value->list ? value->sz_list * sizeof(bool) : sizeof(bool);
            break;
        case xchg_type_int8:
            sz_data = value->list ? value->sz_list * sizeof(int8_t) : sizeof(int8_t);
            break;
        case xchg_type_uint8:
            sz_data = value->list ? value->sz_list * sizeof(uint8_t) : sizeof(uint8_t);
            break;
        case xchg_type_int16:
            sz_data = value->list ? value->sz_list * sizeof(int16_t) : sizeof(int16_t);
            break;
        case xchg_type_uint16:
            sz_data = value->list ? value->sz_list * sizeof(uint16_t) : sizeof(uint16_t);
            break;
        case xchg_type_int32:
            sz_data = value->list ? value->sz_list * sizeof(int32_t) : sizeof(int32_t);
            break;
        case xchg_type_uint32:
            sz_data = value->list ? value->sz_list * sizeof(uint32_t) : sizeof(uint32_t);
            break;
        case xchg_type_int64:
            sz_data = value->list ? value->sz_list * sizeof(int64_t) : sizeof(int64_t);
            break;
        case xchg_type_uint64:
            sz_data = value->list ? value->sz_list * sizeof(uint64_t) : sizeof(uint64_t);
            break;
        case xchg_type_float32:
            sz_data = value->list ? value->sz_list * sizeof(float_t) : sizeof(float_t);
            break;
        case xchg_type_float64:
            sz_data = value->list ? value->sz_list * sizeof(double_t) : sizeof(double_t);
            break;
        default:
            message->error = "value type should be one of xchg_type_t";
            return false;
        }
    }

    if(unlikely(value->sz_data < sz_data))
    {
        message->error = "sz_data is not large enough to contain the specified value";
        return false;
    }

    struct xchg_tag tag = {
        .type = value->type,
        .lsz = value->list ? lsz_from_sz_list(sz_data) : 0,
        .null = value->null,
        .list = value->list,
    };

    uint8_t nr_bytes = value->list ? lsz_to_nr_bytes(tag.lsz) : 0;

    if(unlikely((message->position + sizeof(struct xchg_tag) + nr_bytes + sz_data) > message->length))
    {
        message->error = "the message is not large enough to write the specified value";
        return false;
    }

    memcpy(&((uint8_t *)message->data)[message->position], (uint8_t *)&tag, sizeof(struct xchg_tag));
    message->position += sizeof(struct xchg_tag);

    if(nr_bytes > 0)
    {
        memcpy(&((uint8_t *)message->data)[message->position], (uint8_t *)&value->sz_list, nr_bytes);
        message->position += nr_bytes;
    }

    memcpy(&((uint8_t *)message->data)[message->position], (uint8_t *)value->data, sz_data);
    message->position += sz_data;

    message->error = NULL;
    return true;
}

#define XCHG_MESSAGE_WRITE_HELPERS(xchg_type, c_type)                                                               \
    bool xchg_message_write_##xchg_type(struct xchg_message *message, c_type value)                                 \
    {                                                                                                               \
        assert(message != NULL);                                                                                    \
                                                                                                                    \
        struct xchg_value v = {                                                                                     \
            .type = xchg_type_##xchg_type,                                                                          \
            .null = false,                                                                                          \
            .list = false,                                                                                          \
            .sz_list = 0,                                                                                           \
            .data = (char *)&value,                                                                                 \
            .sz_data = sizeof(c_type),                                                                              \
        };                                                                                                          \
                                                                                                                    \
        return xchg_message_write(message, &v);                                                                     \
    }                                                                                                               \
                                                                                                                    \
    bool xchg_message_write_##xchg_type##_list(struct xchg_message *message, const c_type list[], uint64_t sz_list) \
    {                                                                                                               \
        assert(message != NULL);                                                                                    \
                                                                                                                    \
        struct xchg_value v = {                                                                                     \
            .type = xchg_type_##xchg_type,                                                                          \
            .null = false,                                                                                          \
            .list = true,                                                                                           \
            .sz_list = sz_list,                                                                                     \
            .data = sz_list > 0 ? (char *)list : NULL,                                                              \
            .sz_data = sz_list * sizeof(c_type),                                                                    \
        };                                                                                                          \
                                                                                                                    \
        return xchg_message_write(message, &v);                                                                     \
    }

bool xchg_message_write_null(struct xchg_message *message, enum xchg_type type)
{
    if(unlikely(message == NULL))
    {
        return false;
    }

    struct xchg_value v = {
        .type = type,
        .null = true,
        .list = false,
        .sz_list = 0,
        .data = NULL,
        .sz_data = 0,
    };

    return xchg_message_write(message, &v);
}

bool xchg_message_write_null_list(struct xchg_message *message, enum xchg_type type)
{
    if(unlikely(message == NULL))
    {
        return false;
    }

    struct xchg_value v = {
        .type = type,
        .null = true,
        .list = true,
        .sz_list = 0,
        .data = NULL,
        .sz_data = 0,
    };

    return xchg_message_write(message, &v);
}

XCHG_MESSAGE_WRITE_HELPERS(bool, bool)
XCHG_MESSAGE_WRITE_HELPERS(int8, int8_t)
XCHG_MESSAGE_WRITE_HELPERS(uint8, uint8_t)
XCHG_MESSAGE_WRITE_HELPERS(int16, int16_t)
XCHG_MESSAGE_WRITE_HELPERS(uint16, uint16_t)
XCHG_MESSAGE_WRITE_HELPERS(int32, int32_t)
XCHG_MESSAGE_WRITE_HELPERS(uint32, uint32_t)
XCHG_MESSAGE_WRITE_HELPERS(int64, int64_t)
XCHG_MESSAGE_WRITE_HELPERS(uint64, uint64_t)
XCHG_MESSAGE_WRITE_HELPERS(float32, float_t)
XCHG_MESSAGE_WRITE_HELPERS(float64, double_t)

#undef XCHG_MESSAGE_WRITE_HELPERS

static size_t nr_used(struct xchg_ring *ring, size_t wanted)
{
    if(unlikely(ring == NULL))
    {
        return false;
    }

    size_t used = ring->cw - ring->cr;

    if(used < wanted)
    {
        ring->cw = *ring->w;
        used = ring->cw - ring->cr;
    }

    return used;
}

static size_t nr_free(struct xchg_ring *ring, size_t wanted)
{
    if(unlikely(ring == NULL))
    {
        return false;
    }

    size_t free = ring->cr - ring->cw;

    if(free < wanted)
    {
        ring->cr = *ring->r + ring->sz_data;
        free = ring->cr - ring->cw;
    }

    return free;
}

bool xchg_channel_init(struct xchg_channel *channel,
                       size_t sz_message,
                       char *ingress, size_t sz_ingress,
                       char *egress, size_t sz_egress)
{
    if(unlikely(channel == NULL || (ingress == NULL && egress == NULL)))
    {
        return false;
    }

    if(flp2(sz_message) != sz_message)
    {
        channel->error = "message size is invalid";
        return false;
    }

    size_t sz_ingress_data = sz_ingress - sizeof(size_t) - sizeof(size_t);

    if(ingress != NULL && (flp2(sz_ingress_data) != sz_ingress_data || sz_ingress_data % sz_message != 0))
    {
        channel->error = "ingress size is invalid";
        return false;
    }

    size_t sz_egress_data = sz_egress - sizeof(size_t) - sizeof(size_t);

    if(egress != NULL && (flp2(sz_egress_data) != sz_egress_data || sz_egress_data % sz_message != 0))
    {
        channel->error = "egress size is invalid";
        return false;
    }

    if(ingress != NULL)
    {
        struct xchg_ring *ring = &channel->ingress;

        ring->r = (volatile size_t *)(ingress + (sizeof(size_t) * 0));
        ring->cr = *ring->r;
        ring->w = (volatile size_t *)(ingress + (sizeof(size_t) * 1));
        ring->cw = *ring->w;
        ring->data = ingress + (sizeof(size_t) * 2);
        ring->sz_data = sz_ingress_data;
        ring->sz_message = sz_message;
        ring->mask = sz_ingress_data - 1;
    }

    if(egress != NULL)
    {
        struct xchg_ring *ring = &channel->egress;

        ring->r = (volatile size_t *)(egress + (sizeof(size_t) * 0));
        ring->cr = *ring->r;
        ring->w = (volatile size_t *)(egress + (sizeof(size_t) * 1));
        ring->cw = *ring->w;
        ring->data = egress + (sizeof(size_t) * 2);
        ring->sz_data = sz_egress_data;
        ring->sz_message = sz_message;
        ring->mask = sz_egress_data - 1;
    }

    channel->error = NULL;
    return true;
}

bool xchg_channel_prepare(struct xchg_channel *channel, struct xchg_message *message)
{
    if(unlikely(channel == NULL || message == NULL))
    {
        return false;
    }

    struct xchg_ring *ring = &channel->egress;

    if(unlikely(ring->data == NULL))
    {
        channel->error = "channel has no egress";
        return false;
    }

    if(unlikely(nr_free(ring, ring->sz_message) < ring->sz_message))
    {
        channel->error = "channel is full";
        return false;
    }

    size_t data_offset = ring->cw & ring->mask;
    char *data = ring->data + data_offset;

    xchg_message_init(message, data, ring->sz_message);

    channel->error = NULL;
    return true;
}

bool xchg_channel_send(struct xchg_channel *channel, const struct xchg_message *message)
{
    if(unlikely(channel == NULL || message == NULL))
    {
        return false;
    }

    struct xchg_ring *ring = &channel->egress;

    if(unlikely(ring->data == NULL))
    {
        channel->error = "channel has no egress";
        return false;
    }

    size_t data_offset = ring->cw & ring->mask;
    char *data = ring->data + data_offset;

    if(unlikely(message->length != ring->sz_message || data != message->data))
    {
        channel->error = "message is invalid";
        return false;
    }

    ring->cw += ring->sz_message;
    atomic_thread_fence(memory_order_release);
    *ring->w += ring->sz_message;

    channel->error = NULL;
    return true;
}

bool xchg_channel_receive(struct xchg_channel *channel, struct xchg_message *message)
{
    if(unlikely(channel == NULL || message == NULL))
    {
        return false;
    }

    struct xchg_ring *ring = &channel->ingress;

    if(unlikely(ring->data == NULL))
    {
        channel->error = "channel has no ingress";
        return false;
    }

    if(unlikely(nr_used(ring, ring->sz_message) < ring->sz_message))
    {
        channel->error = "channel is empty";
        return false;
    }

    size_t data_offset = ring->cr & ring->mask;
    char *data = ring->data + data_offset;

    xchg_message_init(message, data, ring->sz_message);

    channel->error = NULL;
    return true;
}

bool xchg_channel_return(struct xchg_channel *channel, const struct xchg_message *message)
{
    if(unlikely(channel == NULL || message == NULL))
    {
        return false;
    }

    struct xchg_ring *ring = &channel->ingress;

    if(unlikely(ring->data == NULL))
    {
        channel->error = "channel has no ingress";
        return false;
    }

    size_t data_offset = ring->cr & ring->mask;
    char *data = ring->data + data_offset;

    if(unlikely(message->length != ring->sz_message || data != message->data))
    {
        channel->error = "message is invalid";
        return false;
    }

    ring->cr += ring->sz_message;
    atomic_thread_fence(memory_order_acquire);
    *ring->r += ring->sz_message;

    channel->error = NULL;
    return true;
}

const char *xchg_channel_strerror(const struct xchg_channel *channel)
{
    if(unlikely(channel == NULL))
    {
        return false;
    }

    return channel->error;
}
