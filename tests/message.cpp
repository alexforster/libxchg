#include <cstring>

#include "catch.hpp"
#include "xchg.h"

TEST_CASE("message lifecycle", "[message]")
{
    char *slab = (char *)"alex forster";
    size_t sz_slab = 12;

    struct xchg_message message = {};
    REQUIRE(xchg_message_init(&message, slab, sz_slab));
    REQUIRE(message.data == slab);
    REQUIRE(message.length == sz_slab);
    REQUIRE(message.position == 0);
    REQUIRE_FALSE(xchg_message_strerror(&message));

    message.position = 4;
    message.error = (char *)0x1;
    REQUIRE(xchg_message_reset(&message));
    REQUIRE(message.data == slab);
    REQUIRE(message.length == sz_slab);
    REQUIRE(message.position == 0);
    REQUIRE_FALSE(xchg_message_strerror(&message));
}

TEST_CASE("message navigation", "[message]")
{
    struct xchg_message message = {
        .data = (char *)"alex forster",
        .length = 12,
    };

    REQUIRE_FALSE(xchg_message_seek(&message, 12));

    REQUIRE(xchg_message_seek(&message, 5));
    size_t position = 0;
    REQUIRE(xchg_message_position(&message, &position));
    REQUIRE(position == 5);
    REQUIRE(memcmp((char *)message.data + message.position, "forster", 7) == 0);

    REQUIRE_FALSE(xchg_message_seek(&message, SIZE_T_MAX));
}

TEST_CASE("message peek", "[message]")
{
    enum xchg_type type = xchg_type_invalid;
    bool null = false;
    bool list = false;
    uint64_t length = 0;

    struct xchg_message message1 = {
        .data = (char *)"\x54\x03\x00\x00\x00\x00\x00\x00\x44\x54\x01\x00\x00",
        .length = 13,
        .position = 0,
        .error = nullptr,
    };

    REQUIRE(xchg_message_peek(&message1, &type, &null, &list, &length));
    REQUIRE(type == xchg_type_int16);
    REQUIRE(!null);
    REQUIRE(list);
    REQUIRE(length == 3);
    REQUIRE(message1.position == 0);

    const size_t message2_size = 1024 * 70;
    struct xchg_message message2 = {
        .data = (char *)malloc(message2_size),
        .length = message2_size,
        .position = 0,
        .error = nullptr,
    };
    bzero(message2.data, message2_size);
    const size_t payload_size = 16500;
    uint32_t payload[payload_size] = {0};
    REQUIRE(xchg_message_write_uint32_list(&message2, payload, payload_size));
    REQUIRE(xchg_message_reset(&message2));
    REQUIRE(xchg_message_peek(&message2, &type, &null, &list, &length));
    REQUIRE(type == xchg_type_uint32);
    REQUIRE(!null);
    REQUIRE(list);
    REQUIRE(length == payload_size);
    REQUIRE(message2.position == 0);
}

template<auto EncodeFn, typename Type>
void xchg_message_write_t(char *slab, struct xchg_message *message)
{
    size_t sz_entry = (sizeof(Type) + 1);
    REQUIRE(xchg_message_init(message, slab, sz_entry * 3));

    REQUIRE(EncodeFn(message, (Type)0));
    REQUIRE(message->position == sz_entry * 1);
    REQUIRE_FALSE(xchg_message_strerror(message));

    REQUIRE(EncodeFn(message, (Type)0));
    REQUIRE(message->position == sz_entry * 2);
    REQUIRE_FALSE(xchg_message_strerror(message));

    REQUIRE(EncodeFn(message, (Type)0));
    REQUIRE(message->position == sz_entry * 3);
    REQUIRE_FALSE(xchg_message_strerror(message));

    REQUIRE_FALSE(EncodeFn(message, (Type)0));
    REQUIRE(message->position == sz_entry * 3);
    REQUIRE(xchg_message_strerror(message));
}

template<auto EncodeFn, typename Type>
void xchg_message_write_t_list(char *slab, struct xchg_message *message)
{
    REQUIRE(xchg_message_init(message, slab, 2 + (3 * sizeof(Type)) + 1 + 2 + sizeof(Type)));

    size_t expected_pos = 2 + (3 * sizeof(Type));
    REQUIRE(EncodeFn(message, (const Type[]) { (Type)0, (Type)0, (Type)0 }, 3));
    REQUIRE(message->position == expected_pos);
    REQUIRE_FALSE(xchg_message_strerror(message));

    expected_pos += 1;
    REQUIRE(EncodeFn(message, (const Type[]) {}, 0));
    REQUIRE(message->position == expected_pos);
    REQUIRE_FALSE(xchg_message_strerror(message));

    expected_pos += 2 + sizeof(Type);
    REQUIRE(EncodeFn(message, (const Type[]) { (Type)0 }, 1));
    REQUIRE(message->position == expected_pos);
    REQUIRE_FALSE(xchg_message_strerror(message));

    REQUIRE_FALSE(EncodeFn(message, (const Type[]) { (Type)0, (Type)0 }, 2));
    REQUIRE(message->position == expected_pos);
    REQUIRE(xchg_message_strerror(message));
}

TEST_CASE("message write", "[message]")
{
    char slab[4096];

    struct xchg_message message = {};

    SECTION("null")
    {
        REQUIRE(xchg_message_init(&message, slab, 3));

        REQUIRE(xchg_message_write_null(&message, xchg_type_int8));
        REQUIRE(message.position == 1);

        REQUIRE(xchg_message_write_null(&message, xchg_type_int16));
        REQUIRE(message.position == 2);

        REQUIRE(xchg_message_write_null(&message, xchg_type_int32));
        REQUIRE(message.position == 3);

        REQUIRE_FALSE(xchg_message_write_null(&message, xchg_type_int64));
        REQUIRE(message.position == 3);
        REQUIRE(xchg_message_strerror(&message));

        REQUIRE(memcmp(message.data, "\x82\x84\x86", message.position) == 0);
    }

    SECTION("null list")
    {
        REQUIRE(xchg_message_init(&message, slab, 3));

        REQUIRE(xchg_message_write_null_list(&message, xchg_type_int8));
        REQUIRE(message.position == 1);

        REQUIRE(xchg_message_write_null_list(&message, xchg_type_int16));
        REQUIRE(message.position == 2);

        REQUIRE(xchg_message_write_null_list(&message, xchg_type_int32));
        REQUIRE(message.position == 3);

        REQUIRE_FALSE(xchg_message_write_null_list(&message, xchg_type_int64));
        REQUIRE(message.position == 3);
        REQUIRE(xchg_message_strerror(&message));

        REQUIRE(memcmp(message.data, "\xc2\xc4\xc6", message.position) == 0);
    }

    SECTION("bool")
    {
        xchg_message_write_t<xchg_message_write_bool, bool>(slab, &message);
        REQUIRE(memcmp(message.data, "\x01\x00\x01\x00\x01\x00", message.position) == 0);
    }

    SECTION("bool list")
    {
        xchg_message_write_t_list<xchg_message_write_bool_list, bool>(slab, &message);
        REQUIRE(memcmp(message.data, "\x51\x03\x00\x00\x00\x41\x51\x01\x00", message.position) == 0);
    }

    SECTION("int8")
    {
        xchg_message_write_t<xchg_message_write_int8, int8_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x02\x00\x02\x00\x02\x00", message.position) == 0);
    }

    SECTION("int8 list")
    {
        xchg_message_write_t_list<xchg_message_write_int8_list, int8_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x52\x03\x00\x00\x00\x42\x52\x01\x00", message.position) == 0);
    }

    SECTION("uint8")
    {
        xchg_message_write_t<xchg_message_write_uint8, uint8_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x03\x00\x03\x00\x03\x00", message.position) == 0);
    }

    SECTION("uint8 list")
    {
        xchg_message_write_t_list<xchg_message_write_uint8_list, uint8_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x53\x03\x00\x00\x00\x43\x53\x01\x00", message.position) == 0);
    }

    SECTION("int16")
    {
        xchg_message_write_t<xchg_message_write_int16, int16_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x04\x00\x00\x04\x00\x00\x04\x00\x00", message.position) == 0);
    }

    SECTION("int16 list")
    {
        xchg_message_write_t_list<xchg_message_write_int16_list, int16_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x54\x03\x00\x00\x00\x00\x00\x00\x44\x54\x01\x00\x00", message.position) == 0);
    }

    SECTION("uint16")
    {
        xchg_message_write_t<xchg_message_write_uint16, uint16_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x05\x00\x00\x05\x00\x00\x05\x00\x00", message.position) == 0);
    }

    SECTION("uint16 list")
    {
        xchg_message_write_t_list<xchg_message_write_uint16_list, uint16_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x55\x03\x00\x00\x00\x00\x00\x00\x45\x55\x01\x00\x00", message.position) == 0);
    }

    SECTION("int32")
    {
        xchg_message_write_t<xchg_message_write_int32, int32_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x06\x00\x00\x00\x00\x06\x00\x00\x00\x00\x06\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("int32 list")
    {
        xchg_message_write_t_list<xchg_message_write_int32_list, int32_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x56\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x46\x56\x01\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("uint32")
    {
        xchg_message_write_t<xchg_message_write_uint32, uint32_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x07\x00\x00\x00\x00\x07\x00\x00\x00\x00\x07\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("uint32 list")
    {
        xchg_message_write_t_list<xchg_message_write_uint32_list, uint32_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x57\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x47\x57\x01\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("int64")
    {
        xchg_message_write_t<xchg_message_write_int64, int64_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x08\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("int64 list")
    {
        xchg_message_write_t_list<xchg_message_write_int64_list, int64_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x58\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x48\x58\x01\x00\x00\x00\x00\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("uint64")
    {
        xchg_message_write_t<xchg_message_write_uint64, uint64_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x09\x00\x00\x00\x00\x00\x00\x00\x00\x09\x00\x00\x00\x00\x00\x00\x00\x00\x09\x00\x00\x00\x00\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("uint64 list")
    {
        xchg_message_write_t_list<xchg_message_write_uint64_list, uint64_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x59\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x49\x59\x01\x00\x00\x00\x00\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("float32")
    {
        xchg_message_write_t<xchg_message_write_float32, float_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x0a\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x0a\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("float32 list")
    {
        xchg_message_write_t_list<xchg_message_write_float32_list, float_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x5a\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x4a\x5a\x01\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("float64")
    {
        xchg_message_write_t<xchg_message_write_float64, double_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x0b\x00\x00\x00\x00\x00\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x00", message.position) == 0);
    }

    SECTION("float64 list")
    {
        xchg_message_write_t_list<xchg_message_write_float64_list, double_t>(slab, &message);
        REQUIRE(memcmp(message.data, "\x5b\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x4b\x5b\x01\x00\x00\x00\x00\x00\x00\x00\x00", message.position) == 0);
    }
}

template<auto DecodeFn, typename Type>
void xchg_message_read_t(struct xchg_message *message)
{
    size_t sz_entry = (sizeof(Type) + 1);
    Type value;

    REQUIRE(DecodeFn(message, &value));
    REQUIRE(message->position == sz_entry * 1);
    REQUIRE(value == (Type)0);

    REQUIRE(DecodeFn(message, &value));
    REQUIRE(message->position == sz_entry * 2);
    REQUIRE(value == (Type)0);

    REQUIRE(DecodeFn(message, &value));
    REQUIRE(message->position == sz_entry * 3);
    REQUIRE(value == (Type)0);

    REQUIRE_FALSE(DecodeFn(message, &value));
    REQUIRE(message->position == sz_entry * 3);
}

template<auto DecodeFn, typename Type>
void xchg_message_read_t_list(struct xchg_message *message)
{
    size_t expected_pos = 0;
    const Type *values = nullptr;
    uint64_t values_len = 0;

    expected_pos += 2 + (3 * sizeof(Type));
    REQUIRE(DecodeFn(message, &values, &values_len));
    REQUIRE(values);
    REQUIRE(values_len == 3);
    REQUIRE(message->position == expected_pos);
    REQUIRE_FALSE(xchg_message_strerror(message));

    expected_pos += 1;
    REQUIRE(DecodeFn(message, &values, &values_len));
    REQUIRE_FALSE(values);
    REQUIRE(values_len == 0);
    REQUIRE(message->position == expected_pos);
    REQUIRE_FALSE(xchg_message_strerror(message));

    expected_pos += 2 + sizeof(Type);
    REQUIRE(DecodeFn(message, &values, &values_len));
    REQUIRE(values);
    REQUIRE(values_len == 1);
    REQUIRE(message->position == expected_pos);
    REQUIRE_FALSE(xchg_message_strerror(message));

    values = nullptr;
    values_len = 0;
    REQUIRE_FALSE(DecodeFn(message, &values, &values_len));
    REQUIRE_FALSE(values);
    REQUIRE(values_len == 0);
    REQUIRE(message->position == expected_pos);
    REQUIRE(xchg_message_strerror(message));
}

TEST_CASE("message read", "[message]")
{
    struct xchg_message message = {};

    SECTION("null")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x82\x84\x86";
        message.length = 3;

        auto type = (enum xchg_type)0;

        REQUIRE_FALSE(xchg_message_read_null_list(&message, &type));
        REQUIRE(type == (enum xchg_type)0);
        REQUIRE(message.position == 0);
        REQUIRE_FALSE(xchg_message_strerror(&message));

        REQUIRE(xchg_message_read_null(&message, &type));
        REQUIRE(type == xchg_type_int8);
        REQUIRE(message.position == 1);

        REQUIRE(xchg_message_read_null(&message, &type));
        REQUIRE(type == xchg_type_int16);
        REQUIRE(message.position == 2);

        REQUIRE(xchg_message_read_null(&message, &type));
        REQUIRE(type == xchg_type_int32);
        REQUIRE(message.position == 3);

        REQUIRE_FALSE(xchg_message_read_null(&message, &type));
        REQUIRE(message.position == 3);
    }

    SECTION("null list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\xc2\xc4\xc6";
        message.length = 3;

        auto type = (enum xchg_type)0;

        REQUIRE_FALSE(xchg_message_read_null(&message, &type));
        REQUIRE(type == (enum xchg_type)0);
        REQUIRE(message.position == 0);
        REQUIRE_FALSE(xchg_message_strerror(&message));

        REQUIRE(xchg_message_read_null_list(&message, &type));
        REQUIRE(type == xchg_type_int8);
        REQUIRE(message.position == 1);

        REQUIRE(xchg_message_read_null_list(&message, &type));
        REQUIRE(type == xchg_type_int16);
        REQUIRE(message.position == 2);

        REQUIRE(xchg_message_read_null_list(&message, &type));
        REQUIRE(type == xchg_type_int32);
        REQUIRE(message.position == 3);

        REQUIRE_FALSE(xchg_message_read_null_list(&message, &type));
        REQUIRE(message.position == 3);
    }

    SECTION("bool")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x01\x00\x01\x00\x01\x00";
        message.length = 6;
        xchg_message_read_t<xchg_message_read_bool, bool>(&message);
    }

    SECTION("bool list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x51\x03\x00\x00\x00\x41\x51\x01\x00";
        message.length = 9;
        xchg_message_read_t_list<xchg_message_read_bool_list, bool>(&message);
    }

    SECTION("int8")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x02\x00\x02\x00\x02\x00";
        message.length = 6;
        xchg_message_read_t<xchg_message_read_int8, int8_t>(&message);
    }

    SECTION("int8 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x52\x03\x00\x00\x00\x42\x52\x01\x00";
        message.length = 9;
        xchg_message_read_t_list<xchg_message_read_int8_list, int8_t>(&message);
    }

    SECTION("uint8")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x03\x00\x03\x00\x03\x00";
        message.length = 6;
        xchg_message_read_t<xchg_message_read_uint8, uint8_t>(&message);
    }

    SECTION("uint8 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x53\x03\x00\x00\x00\x43\x53\x01\x00";
        message.length = 9;
        xchg_message_read_t_list<xchg_message_read_uint8_list, uint8_t>(&message);
    }

    SECTION("int16")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x04\x00\x00\x04\x00\x00\x04\x00\x00";
        message.length = 9;
        xchg_message_read_t<xchg_message_read_int16, int16_t>(&message);
    }

    SECTION("int16 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x54\x03\x00\x00\x00\x00\x00\x00\x44\x54\x01\x00\x00";
        message.length = 13;
        xchg_message_read_t_list<xchg_message_read_int16_list, int16_t>(&message);
    }

    SECTION("uint16")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x05\x00\x00\x05\x00\x00\x05\x00\x00";
        message.length = 9;
        xchg_message_read_t<xchg_message_read_uint16, uint16_t>(&message);
    }

    SECTION("uint16 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x55\x03\x00\x00\x00\x00\x00\x00\x45\x55\x01\x00\x00";
        message.length = 13;
        xchg_message_read_t_list<xchg_message_read_uint16_list, uint16_t>(&message);
    }

    SECTION("int32")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x06\x00\x00\x00\x00\x06\x00\x00\x00\x00\x06\x00\x00\x00\x00";
        message.length = 15;
        xchg_message_read_t<xchg_message_read_int32, int32_t>(&message);
    }

    SECTION("int32 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x56\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x46\x56\x01\x00\x00\x00\x00";
        message.length = 21;
        xchg_message_read_t_list<xchg_message_read_int32_list, int32_t>(&message);
    }

    SECTION("uint32")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x07\x00\x00\x00\x00\x07\x00\x00\x00\x00\x07\x00\x00\x00\x00";
        message.length = 15;
        xchg_message_read_t<xchg_message_read_uint32, uint32_t>(&message);
    }

    SECTION("uint32 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x57\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x47\x57\x01\x00\x00\x00\x00";
        message.length = 21;
        xchg_message_read_t_list<xchg_message_read_uint32_list, uint32_t>(&message);
    }

    SECTION("int64")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x08\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00";
        message.length = 27;
        xchg_message_read_t<xchg_message_read_int64, int64_t>(&message);
    }

    SECTION("int64 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x58\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x48\x58\x01\x00\x00\x00\x00\x00\x00\x00\x00";
        message.length = 37;
        xchg_message_read_t_list<xchg_message_read_int64_list, int64_t>(&message);
    }

    SECTION("uint64")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x09\x00\x00\x00\x00\x00\x00\x00\x00\x09\x00\x00\x00\x00\x00\x00\x00\x00\x09\x00\x00\x00\x00\x00\x00\x00\x00";
        message.length = 27;
        xchg_message_read_t<xchg_message_read_uint64, uint64_t>(&message);
    }

    SECTION("uint64 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x59\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x49\x59\x01\x00\x00\x00\x00\x00\x00\x00\x00";
        message.length = 37;
        xchg_message_read_t_list<xchg_message_read_uint64_list, uint64_t>(&message);
    }

    SECTION("float32")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x0a\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x0a\x00\x00\x00\x00";
        message.length = 15;
        xchg_message_read_t<xchg_message_read_float32, float_t>(&message);
    }

    SECTION("float32 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x5a\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x4a\x5a\x01\x00\x00\x00\x00";
        message.length = 21;
        xchg_message_read_t_list<xchg_message_read_float32_list, float_t>(&message);
    }

    SECTION("float64")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x0b\x00\x00\x00\x00\x00\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x00";
        message.length = 27;
        xchg_message_read_t<xchg_message_read_float64, double_t>(&message);
    }

    SECTION("float64 list")
    {
        bzero(&message, sizeof(struct xchg_message));
        message.data = (char *)"\x5b\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x4b\x5b\x01\x00\x00\x00\x00\x00\x00\x00\x00";
        message.length = 37;
        xchg_message_read_t_list<xchg_message_read_float64_list, double_t>(&message);
    }
}
