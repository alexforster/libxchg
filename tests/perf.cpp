#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>
#include <queue>
#include <random>
#include <chrono>
#include <thread>

#include "catch.hpp"
#include "xchg.h"

#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)

using namespace std;

struct period
{
    period() :
        start(chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch())),
        count(0),
        total(chrono::steady_clock::duration::zero()),
        min(chrono::steady_clock::duration::max()),
        max(chrono::steady_clock::duration::min()),
        average(chrono::steady_clock::duration::zero()),
        histogram {}
    {
    }

    chrono::steady_clock::time_point start;

    uint64_t count;

    chrono::steady_clock::duration total;

    chrono::steady_clock::duration min;

    chrono::steady_clock::duration max;

    chrono::steady_clock::duration average;

    array<uint64_t, 19> histogram;
};

class histogram
{
private:
    chrono::steady_clock::time_point mark;

    shared_ptr<period> current_period;

    unique_ptr<period> next_period;

public:
    const array<uint64_t, 19> ranks = {
        8,
        16,
        32,
        64,
        128,
        256,
        512,
        1000,
        2000,
        4000,
        8000,
        16000,
        32000,
        64000,
        128000,
        256000,
        512000,
        1000000,
        numeric_limits<uint64_t>::max(),
    };

    const string name;

    explicit histogram(string name) :
        name(move(name))
    {
        this->current_period = nullptr;
        this->next_period = make_unique<period>();
    }

    void start()
    {
        auto now = chrono::steady_clock::now();

        if(now >= this->next_period->start + chrono::seconds(1))
        {
            this->current_period = move(this->next_period);
            this->next_period = make_unique<period>();

            auto nr_units = this->current_period->count;
            auto duration_ns = chrono::duration_cast<chrono::nanoseconds>(this->current_period->total).count();

            if(nr_units > 0)
            {
                this->current_period->average = chrono::steady_clock::duration(chrono::nanoseconds(duration_ns / nr_units));
            }
            else
            {
                this->current_period->average = chrono::steady_clock::duration(chrono::nanoseconds(duration_ns));
            }

            now = chrono::steady_clock::now();
        }

        this->mark = now;
    }

    void stop(uint64_t nr_units = 1)
    {
        if(nr_units == 0)
        {
            return;
        }

        auto total_duration = chrono::steady_clock::now() - this->mark;
        auto unit_duration_ns = total_duration.count() / nr_units;
        auto unit_duration = chrono::steady_clock::duration(chrono::nanoseconds(unit_duration_ns));

        this->next_period->count += nr_units;
        this->next_period->total += total_duration;

        if(unit_duration < this->next_period->min)
        {
            this->next_period->min = unit_duration;
        }

        if(unit_duration > this->next_period->max)
        {
            this->next_period->max = unit_duration;
        }

        for(int i = 0; i < 19; i++)
        {
            if(unit_duration_ns < this->ranks[i])
            {
                this->next_period->histogram[i] += nr_units;
                break;
            }
        }
    }

    shared_ptr<period> get()
    {
        return this->current_period;
    }
};

string rank_to_string(uint64_t ns)
{
    if(ns >= 1000000000)
    {
        return to_string((uint64_t)floor(ns / 1000000000.0)) + " s";
    }
    else if(ns >= 1000000)
    {
        return to_string((uint64_t)floor(ns / 1000000.0)) + " ms";
    }
    else if(ns >= 1000)
    {
        return to_string((uint64_t)floor(ns / 1000.0)) + " us";
    }
    else
    {
        return to_string(ns) + " ns";
    }
}

string histogram_to_string(histogram &histogram)
{
    auto period = histogram.get();

    stringstream ss;

    if(period != nullptr)
    {
        ss << histogram.name << ": ";
        ss << period->count << " ops/sec @ " << period->average.count() << "ns/op";
        ss << endl;

        for(size_t i = 0; i < histogram.ranks.size(); i++)
        {
            if(i == histogram.ranks.size() - 1)
            {
                ss << left << setw(10) << (rank_to_string(histogram.ranks[i - 1]) + "+");
            }
            else
            {
                ss << left << setw(10) << ("<" + rank_to_string(histogram.ranks[i]));
            }
        }
        ss << endl;

        for(auto count : period->histogram)
        {
            auto pct = (uint64_t)roundl(((double_t)count / (double_t)period->count) * 100.0);
            ss << left << setw(10) << (" " + to_string(pct) + "%");
        }
        ss << endl;

        for(auto count : period->histogram)
        {
            ss << left << setw(10) << (" " + to_string(count));
        }
        ss << endl;
    }

    return ss.str();
}

void perf_main(struct xchg_channel *channel, histogram *tx_histogram, histogram *rx_histogram)
{
    assert(channel != nullptr);
    assert(tx_histogram != nullptr);
    assert(rx_histogram != nullptr);

    random_device r;
    mt19937_64 e1(r());
    uniform_int_distribution<size_t> uniform_dist(2, 16);

    auto start = chrono::steady_clock::now();
    auto end = start + chrono::milliseconds(2100);

    size_t nr;
    uint64_t send_counter = 0;
    uint64_t recv_counter = 0;

    xchg_message message = {};

    for(size_t x = 0; (x % 16384 != 0) || chrono::steady_clock::now() < end; x++)
    {
        tx_histogram->start();
        for(nr = 0; nr < uniform_dist(e1); nr++)
        {
            if(!xchg_channel_prepare(channel, &message))
            {
                break;
            }
            if(unlikely(!xchg_message_write_uint64(&message, send_counter++)))
            {
                FAIL("xchg_message_write_uint64");
            }
            if(unlikely(!xchg_channel_send(channel, &message)))
            {
                FAIL("xchg_channel_send");
            }
        }
        tx_histogram->stop(nr);

        rx_histogram->start();
        for(nr = 0; nr < uniform_dist(e1); nr++)
        {
            if(!xchg_channel_receive(channel, &message))
            {
                break;
            }
            uint64_t counter = 0;
            if(unlikely(!xchg_message_read_uint64(&message, &counter)))
            {
                FAIL("xchg_message_read_uint64");
            }
            if(unlikely(counter != recv_counter++))
            {
                FAIL("counter == recv_counter");
            }
            if(unlikely(!xchg_channel_return(channel, &message)))
            {
                FAIL("xchg_channel_return");
            }
        }
        rx_histogram->stop(nr);
    }
}

TEST_CASE("perf")
{
    char client_slab[(64 * 1024) + 16] = {};
    char server_slab[(64 * 1024) + 16] = {};

    xchg_channel client = {};
    xchg_channel_init(&client, 16, client_slab, sizeof(client_slab), server_slab, sizeof(server_slab));

    xchg_channel server = {};
    xchg_channel_init(&server, 16, server_slab, sizeof(server_slab), client_slab, sizeof(client_slab));

    auto ctxh = histogram("Client Tx");
    auto crxh = histogram("Client Rx");
    auto stxh = histogram("Server Tx");
    auto srxh = histogram("Server Rx");

    auto client_thread = thread(perf_main, &client, &ctxh, &srxh);
    auto server_thread = thread(perf_main, &server, &stxh, &crxh);

    client_thread.join();
    server_thread.join();

    cerr << histogram_to_string(ctxh);
    cerr << endl;
    cerr << histogram_to_string(crxh);
    cerr << endl;
    cerr << histogram_to_string(stxh);
    cerr << endl;
    cerr << histogram_to_string(srxh);
    cerr << endl;

    auto sum_count = ctxh.get()->count + crxh.get()->count + stxh.get()->count + srxh.get()->count;
    auto sum_average = (ctxh.get()->average.count() +
                        crxh.get()->average.count() +
                        stxh.get()->average.count() +
                        srxh.get()->average.count());

    cerr.imbue(locale(""));
    cerr << "Total: " << fixed << sum_count << " ops/sec @ " << to_string(sum_average / 4) << "ns/op" << endl;
    cerr << endl;

    SUCCEED("no data races detected");
}
