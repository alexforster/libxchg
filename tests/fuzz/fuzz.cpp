#include <cstddef>
#include <cstdio>
#include <cassert>
#include <cstring>

#include "xchg.h"

template<typename T, enum xchg_type XchgTypeT, bool (*FnValueT)(xchg_message *, T *), bool (*FnListT)(xchg_message *, const T *[], uint32_t *)>
bool consume_message(struct xchg_message &message, enum xchg_type type, bool null, bool list, uint32_t sz_list)
{
    if(list)
    {
        if(null)
        {
            enum xchg_type type_;
            assert(xchg_message_read_null_list(&message, &type_));
            assert(type_ == XchgTypeT);
            assert(type_ == type);
        }
        else
        {
            const T *list_;
            uint32_t sz_list_;
            assert(FnListT(&message, &list_, &sz_list_));
            assert((sz_list_ > 0) ? (list_ != nullptr) : (list_ == nullptr));
            assert(sz_list_ == sz_list);
        }
    }
    else
    {
        if(null)
        {
            enum xchg_type type_;
            assert(xchg_message_read_null(&message, &type_));
            assert(type_ == XchgTypeT);
            assert(type_ == type);
        }
        else
        {
            T value;
            assert(FnValueT(&message, &value));
        }
    }

    return true;
}

void fuzz_message(char *input, size_t sz_input)
{
    struct xchg_message message = {};
    if(!xchg_message_init(&message, input, sz_input))
    {
        assert(false);  // only fails if provided invalid arguments
    }

    enum xchg_type type;
    bool null;
    bool list;
    uint32_t sz_list;
    while(xchg_message_peek(&message, &type, &null, &list, &sz_list))
    {
        switch(type)
        {
        case xchg_type_bool:
            if(!consume_message<bool, xchg_type_bool, xchg_message_read_bool, xchg_message_read_bool_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_int8:
            if(!consume_message<int8_t, xchg_type_int8, xchg_message_read_int8, xchg_message_read_int8_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_uint8:
            if(!consume_message<uint8_t, xchg_type_uint8, xchg_message_read_uint8, xchg_message_read_uint8_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_int16:
            if(!consume_message<int16_t, xchg_type_int16, xchg_message_read_int16, xchg_message_read_int16_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_uint16:
            if(!consume_message<uint16_t, xchg_type_uint16, xchg_message_read_uint16, xchg_message_read_uint16_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_int32:
            if(!consume_message<int32_t, xchg_type_int32, xchg_message_read_int32, xchg_message_read_int32_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_uint32:
            if(!consume_message<uint32_t, xchg_type_uint32, xchg_message_read_uint32, xchg_message_read_uint32_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_int64:
            if(!consume_message<int64_t, xchg_type_int64, xchg_message_read_int64, xchg_message_read_int64_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_uint64:
            if(!consume_message<uint64_t, xchg_type_uint64, xchg_message_read_uint64, xchg_message_read_uint64_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_float32:
            if(!consume_message<float_t, xchg_type_float32, xchg_message_read_float32, xchg_message_read_float32_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        case xchg_type_float64:
            if(!consume_message<double_t, xchg_type_float64, xchg_message_read_float64, xchg_message_read_float64_list>(message, type, null, list, sz_list))
            {
                continue;
            }
            break;
        default:
            assert(false);  // xchg_message_peek should not have returned true
        }
    }
}

int main(int argc, char *argv[])
{
    FILE *fd;
    if(argc > 1)
    {
        fd = fopen(argv[1], "rb");
        if(fd == nullptr)
        {
            perror("fopen");
            abort();
        }
    }
    else
    {
        fd = freopen(nullptr, "rb", stdin);
        if(fd == nullptr)
        {
            perror("freopen");
            abort();
        }
    }

#ifdef __AFL_LOOP
    while(__AFL_LOOP(1000))
    {
#endif
        char input[64 * 1024];
        auto sz_input = fread(input, 1, sizeof(input), fd);
        fuzz_message(input, sz_input);
#ifdef __AFL_LOOP
    }
#endif

    return 0;
}
