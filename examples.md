# Examples

See [`examples/serialization.c`](https://github.com/alexforster/libxchg/tree/master/examples/serialization.c) for a more complete example.

#### Setting up a new xchg_channel

```c
const size_t MESSAGE_SIZE = 64;

const size_t RING_SIZE = 4096 + (2 * sizeof(void *));

char *ring_a = malloc(RING_SIZE); // pretend this is some kind of shared memory
char *ring_b = malloc(RING_SIZE);

struct xchg_channel channel = {};

if(!xchg_channel_init(&channel, MESSAGE_SIZE, ring_a, RING_SIZE, ring_b, RING_SIZE))
{
    printf("xchg_channel_init: %s\n", xchg_channel_strerror(&channel));
    return;
}
```

#### Sending an xchg_message to an xchg_channel

```c
struct xchg_message message = {};

if(!xchg_channel_prepare(channel, &message)) {
    printf("xchg_channel_prepare: %s\n", xchg_channel_strerror(channel));
    return;
}

if(!xchg_message_write_uint16(message, event->type)) {
    printf("xchg_message_write_uint16: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_message_write_uint64(message, event->identifier)) {
    printf("xchg_message_write_uint64: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_message_write_int32(message, event->position_x)) {
    printf("xchg_message_write_int32: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_message_write_int32(message, event->position_y)) {
    printf("xchg_message_write_int32: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_message_write_float32(message, event->direction)) {
    printf("xchg_message_write_float32: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_message_write_float32(message, event->velocity)) {
    printf("xchg_message_write_float32: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_message_write_float32(message, event->force)) {
    printf("xchg_message_write_float32: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_channel_send(channel, &message)) {
    printf("xchg_channel_send: %s\n", xchg_channel_strerror(channel));
    return;
}
```

#### Receiving an xchg_message from an xchg_channel

```c
struct xchg_message message = {};

if(!xchg_channel_receive(channel, &message)) {
    return;
}

uint16_t event_type = 0;
if(!xchg_message_read_uint16(message, &event_type)) {
    printf("xchg_message_read_uint16: %s\n", xchg_message_strerror(channel));
    return;
}

uint64_t identifier = 0;
if(!xchg_message_read_uint64(message, &identifier)) {
    printf("xchg_message_read_uint64: %s\n", xchg_message_strerror(channel));
    return;
}

int32_t position_x = 0;
if(!xchg_message_read_int32(message, &position_x)) {
    printf("xchg_message_read_int32: %s\n", xchg_message_strerror(channel));
    return;
}

int32_t position_y = 0;
if(!xchg_message_read_int32(message, &position_y)) {
    printf("xchg_message_read_int32: %s\n", xchg_message_strerror(channel));
    return;
}

float_t direction = 0.0;
if(!xchg_message_read_float32(message, &direction)) {
    printf("xchg_message_read_float32: %s\n", xchg_message_strerror(channel));
    return;
}

float_t velocity = 0.0;
if(!xchg_message_read_float32(message, &velocity)) {
    printf("xchg_message_read_float32: %s\n", xchg_message_strerror(channel));
    return;
}

float_t force = 0.0;
if(!xchg_message_read_float32(message, &force)) {
    printf("xchg_message_read_float32: %s\n", xchg_message_strerror(channel));
    return;
}

if(!xchg_channel_return(channel, &message)) {
    printf("xchg_channel_return: %s\n", xchg_channel_strerror(channel));
    return;
}
```
