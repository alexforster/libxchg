#include <cstring>

#include "catch.hpp"
#include "xchg.h"

TEST_CASE("channel create", "[channel]")
{
    char slab_a[4112] = {};
    char slab_b[4112] = {};

    struct xchg_channel channel = {};

    for(size_t i = 0; i < 64; i++)
    {
        REQUIRE_FALSE(xchg_channel_init(&channel, 64, slab_a, i, nullptr, 0));
        REQUIRE(xchg_channel_strerror(&channel));
        REQUIRE_FALSE(xchg_channel_init(&channel, 64, nullptr, 0, slab_b, i));
        REQUIRE(xchg_channel_strerror(&channel));
    }

    REQUIRE(xchg_channel_init(&channel, 64, slab_a, sizeof(slab_a), slab_b, sizeof(slab_b)));
    REQUIRE_FALSE(xchg_channel_strerror(&channel));
    REQUIRE(channel.ingress.sz_data == 4096);
    REQUIRE(*channel.ingress.r == 0);
    REQUIRE(*channel.ingress.w == 0);
    REQUIRE(channel.ingress.cr == 0);
    REQUIRE(channel.ingress.cw == 0);
    REQUIRE(channel.egress.sz_data == 4096);
    REQUIRE(*channel.egress.r == 0);
    REQUIRE(*channel.egress.w == 0);
    REQUIRE(channel.egress.cr == 0);
    REQUIRE(channel.egress.cw == 0);
}

TEST_CASE("channel receive empty", "[channel]")
{
    char slab[4112] = {};

    struct xchg_channel channel_a = {};
    REQUIRE(xchg_channel_init(&channel_a, 64, nullptr, 0, slab, sizeof(slab)));
    struct xchg_channel channel_b = {};
    REQUIRE(xchg_channel_init(&channel_b, 64, slab, sizeof(slab), nullptr, 0));

    struct xchg_message message = {};
    REQUIRE_FALSE(xchg_channel_receive(&channel_b, &message));
    REQUIRE(channel_b.ingress.cr == 0);
    REQUIRE(*channel_b.ingress.r == 0);
    REQUIRE(channel_b.ingress.cw == 0);
    REQUIRE(*channel_b.ingress.w == 0);
}

TEST_CASE("channel receive full", "[channel]")
{
    char slab[144] = {};

    struct xchg_channel channel_a = {};
    REQUIRE(xchg_channel_init(&channel_a, 64, nullptr, 0, slab, sizeof(slab)));
    struct xchg_channel channel_b = {};
    REQUIRE(xchg_channel_init(&channel_b, 64, slab, sizeof(slab), nullptr, 0));

    struct xchg_message message = {};
    REQUIRE(xchg_channel_prepare(&channel_a, &message));
    xchg_message_write_uint8_list(&message, (const uint8_t *)"alex forster", 12);
    REQUIRE(xchg_channel_send(&channel_a, &message));
    REQUIRE(xchg_channel_prepare(&channel_a, &message));
    xchg_message_write_uint8_list(&message, (const uint8_t *)"alex forster", 12);
    REQUIRE(xchg_channel_send(&channel_a, &message));

    REQUIRE_FALSE(xchg_channel_prepare(&channel_a, &message));
}

TEST_CASE("channel send too small", "[channel]")
{
    char slab[144] = {};

    struct xchg_channel channel_a = {};
    REQUIRE(xchg_channel_init(&channel_a, 64, nullptr, 0, slab, sizeof(slab)));
    struct xchg_channel channel_b = {};
    REQUIRE(xchg_channel_init(&channel_b, 64, slab, sizeof(slab), nullptr, 0));

    struct xchg_message message = {};

    REQUIRE(xchg_channel_prepare(&channel_a, &message));
    xchg_message_write_uint8_list(&message, (const uint8_t *)"alex forster", 12);

    REQUIRE(xchg_channel_send(&channel_a, &message));
    REQUIRE(channel_a.egress.cr == 128);
    REQUIRE(*channel_a.egress.r == 0);
    REQUIRE(channel_a.egress.cw == 64);
    REQUIRE(*channel_a.egress.w == 64);

    REQUIRE(xchg_channel_prepare(&channel_a, &message));
    xchg_message_write_uint8_list(&message, (const uint8_t *)"alex forster", 12);

    REQUIRE(xchg_channel_send(&channel_a, &message));
    REQUIRE(channel_a.egress.cr == 128);
    REQUIRE(*channel_a.egress.r == 0);
    REQUIRE(channel_a.egress.cw == 128);
    REQUIRE(*channel_a.egress.w == 128);

    REQUIRE_FALSE(xchg_channel_prepare(&channel_a, &message));
}

TEST_CASE("channel send/receive wraparound", "[channel]")
{
    char slab[4112] = {};

    struct xchg_channel channel_a = {};
    REQUIRE(xchg_channel_init(&channel_a, 64, nullptr, 0, slab, sizeof(slab)));
    struct xchg_channel channel_b = {};
    REQUIRE(xchg_channel_init(&channel_b, 64, slab, sizeof(slab), nullptr, 0));

    for(size_t i = 0; i < 96; i++)
    {
        struct xchg_message message = {};
        REQUIRE(xchg_channel_prepare(&channel_a, &message));
        xchg_message_write_uint8_list(&message, (const uint8_t *)"alex forster", 12);
        REQUIRE(xchg_channel_send(&channel_a, &message));

        REQUIRE(xchg_channel_receive(&channel_b, &message));
        uint64_t message_length = 0;
        const uint8_t *message_payload = nullptr;
        xchg_message_read_uint8_list(&message, &message_payload, &message_length);
        REQUIRE(memcmp(message_payload, "alex forster", message_length) == 0);
        REQUIRE(xchg_channel_return(&channel_b, &message));
    }

    REQUIRE(channel_a.egress.cr == 8192);
    REQUIRE(*channel_a.egress.r == 6144);
    REQUIRE(channel_a.egress.cw == 6144);
    REQUIRE(*channel_a.egress.w == 6144);

    REQUIRE(channel_b.ingress.cr == 6144);
    REQUIRE(*channel_b.ingress.r == 6144);
    REQUIRE(channel_b.ingress.cw == 6144);
    REQUIRE(*channel_b.ingress.w == 6144);
}
