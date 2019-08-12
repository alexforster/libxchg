#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#include "xchg.h"

enum event_type : uint16_t
{
    event_type_touch_down,
    event_type_touch_drag,
    event_type_touch_up,
    event_type_unknown,
};

const char *event_type_strings[] = {
    "event_type_unknown",
    "event_type_touch_down",
    "event_type_touch_drag",
    "event_type_touch_up",
};

struct event
{
    enum event_type type;
};

struct touch_event
{
    enum event_type type;

    uint64_t identifier;
    int32_t position_x;
    int32_t position_y;
    float_t direction;
    float_t velocity;
    float_t force;
};

bool serialize_event(struct event *event, struct xchg_message *message)
{
    assert(event != NULL);
    assert(message != NULL);

    if(!xchg_message_write_uint16(message, event->type))
        return false;

    switch(event->type)
    {
    case event_type_touch_down:
    case event_type_touch_drag:
    case event_type_touch_up:
    {
        struct touch_event *touch_event = (struct touch_event *)event;

        if(!xchg_message_write_uint64(message, touch_event->identifier))
            return false;
        if(!xchg_message_write_int32(message, touch_event->position_x))
            return false;
        if(!xchg_message_write_int32(message, touch_event->position_y))
            return false;
        if(!xchg_message_write_float32(message, touch_event->direction))
            return false;
        if(!xchg_message_write_float32(message, touch_event->velocity))
            return false;
        if(!xchg_message_write_float32(message, touch_event->force))
            return false;

        return true;
    }
    default:
        return false;
    }
}

bool deserialize_event(struct xchg_message *message, struct event **event)
{
    assert(message != NULL);
    assert(event != NULL);

    uint16_t event_type = 0;

    if(!xchg_message_read_uint16(message, &event_type))
        return false;

    switch(event_type)
    {
    case event_type_touch_up:
    case event_type_touch_drag:
    case event_type_touch_down:
    {
        struct touch_event touch_event = {};

        touch_event.type = event_type;

        if(!xchg_message_read_uint64(message, &touch_event.identifier))
            return false;
        if(!xchg_message_read_int32(message, &touch_event.position_x))
            return false;
        if(!xchg_message_read_int32(message, &touch_event.position_y))
            return false;
        if(!xchg_message_read_float32(message, &touch_event.direction))
            return false;
        if(!xchg_message_read_float32(message, &touch_event.velocity))
            return false;
        if(!xchg_message_read_float32(message, &touch_event.force))
            return false;

        *event = malloc(sizeof(touch_event));
        memcpy(*event, &touch_event, sizeof(touch_event));

        return true;
    }
    default:
        return false;
    }
}

bool running = true;

void stop(int signum)
{
    running = false;
}

void client_thread_main(struct xchg_channel *channel);
void server_thread_main(struct xchg_channel *channel);

int main(int argc, char *argv[])
{
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    signal(SIGQUIT, stop);

    size_t sz_ring = 4096 + (2 * sizeof(void *));

    char *ring_a = malloc(sz_ring);
    char *ring_b = malloc(sz_ring);

    struct xchg_channel client_channel = {};
    struct xchg_channel server_channel = {};

    if(!xchg_channel_init(&client_channel, 64, ring_a, sz_ring, ring_b, sz_ring))
    {
        printf("xchg_channel_init: %s\n", xchg_channel_strerror(&client_channel));
        return 1;
    }

    if(!xchg_channel_init(&server_channel, 64, ring_b, sz_ring, ring_a, sz_ring))
    {
        printf("xchg_channel_init: %s\n", xchg_channel_strerror(&client_channel));
        return 1;
    }

    sigset_t empty_sigset, original_sigset;
    sigemptyset(&empty_sigset);
    pthread_sigmask(SIG_SETMASK, &empty_sigset, &original_sigset);

    pthread_t client_thread = 0;
    pthread_t server_thread = 0;

    if((errno = pthread_create(&client_thread, NULL, (void *)client_thread_main, (void *)&client_channel)) != 0)
    {
        printf("pthread_create: %s\n", strerror(errno));
        return 1;
    }

    if((errno = pthread_create(&server_thread, NULL, (void *)server_thread_main, (void *)&server_channel)) != 0)
    {
        printf("pthread_create: %s\n", strerror(errno));
        return 1;
    }

    pthread_sigmask(SIG_SETMASK, &original_sigset, NULL);

    while(running)
    {
        nanosleep((const struct timespec[]) { { 0, 10000000 /*10ms*/ } }, NULL);
    }

    pthread_cancel(server_thread);
    pthread_cancel(client_thread);

    if((errno = pthread_join(server_thread, NULL)) != 0)
    {
        printf("pthread_join: %s\n", strerror(errno));
        return 1;
    }

    if((errno = pthread_join(client_thread, NULL)) != 0)
    {
        printf("pthread_join: %s\n", strerror(errno));
        return 1;
    }

    free(ring_a);
    ring_a = NULL;

    free(ring_b);
    ring_b = NULL;

    return 0;
}

void client_thread_main(struct xchg_channel *channel)
{
    while(running)
    {
        struct touch_event touch_event = {
            .type = event_type_touch_drag,
            .identifier = 0xDEADBEEF,
            .position_x = 1270,
            .position_y = 664,
            .direction = 204.7,
            .velocity = 0.2741058,
            .force = 1.0,
        };

        struct xchg_message message = {};

        if(!xchg_channel_prepare(channel, &message))
        {
            printf("xchg_channel_prepare: %s\n", xchg_channel_strerror(channel));
            continue;
        }

        if(!serialize_event((struct event *)&touch_event, &message))
        {
            printf("serialize_event: %s\n", xchg_message_strerror(&message));
            xchg_message_reset(&message);
            continue;
        }

        if(!xchg_channel_send(channel, &message))
        {
            printf("xchg_channel_send: %s\n", xchg_channel_strerror(channel));
            return;
        }

        nanosleep((const struct timespec[]) { { 1, 0} }, NULL);
    }
}

void server_thread_main(struct xchg_channel *channel)
{
    while(running)
    {
        struct xchg_message message = {};

        if(!xchg_channel_receive(channel, &message))
        {
            nanosleep((const struct timespec[]) { { 0, 10000000 /*10ms*/ } }, NULL);
            continue;
        }

        struct event *event = NULL;

        if(deserialize_event(&message, &event))
        {
            event->type = (event->type > event_type_unknown) ? event_type_unknown : event->type;

            switch(event->type)
            {
            case event_type_touch_up:
            case event_type_touch_drag:
            case event_type_touch_down:
            {
                struct touch_event *touch_event = (struct touch_event *)event;

                printf("[%s] id=%llu; x=%d; y=%d; d=%f; v=%f; f=%f\n",
                       event_type_strings[touch_event->type], touch_event->identifier,
                       touch_event->position_x, touch_event->position_y,
                       touch_event->direction, touch_event->velocity, touch_event->force);
                break;
            }
            default:
            {
                printf("[%s]\n", event_type_strings[event->type]);
                break;
            }
            }

            free(event);
            event = NULL;
        }
        else
        {
            printf("deserialize_event: %s\n", xchg_message_strerror(&message));
        }

        if(!xchg_channel_return(channel, &message))
        {
            printf("xchg_channel_return: %s\n", xchg_channel_strerror(channel));
            return;
        }
    }
}
