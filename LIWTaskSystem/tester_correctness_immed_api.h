#pragma once
#include <iostream>
#include "LIWThreadSafeQueueSized.h"
#include <thread>
#include <atomic>

using namespace std;
using namespace LIW;
using namespace LIW::Util;

std::atomic<uint32_t> counter[65536];
LIWThreadSafeQueueSized<uint16_t, 3> q;

void tester_correctness_immed_api()
{
    std::atomic<uint16_t> seq;
    seq = 0;
    static const int PRODUCE_N = 4;
    static const int CONSUMER_N = 4;
    static const int MULTIPLIER = 10;
    std::atomic<int> finished_producer;
    finished_producer = 0;
    auto producer = [&seq, &finished_producer]()
    {
        for (int i = 0; i < 65536 * MULTIPLIER; i++)
        {
            uint16_t s = seq++;
            while (!q.push_now(s));
        }
        finished_producer++;
    };

    for (int i = 0; i < 65536; i++)
    {
        counter[i] = 0;
    }
    auto consumer = [&finished_producer]()
    {
        uint16_t s = 0;
        while (finished_producer < PRODUCE_N)
        {
            if (q.pop_now(s))
            {
                counter[s]++;
            }
        }
        while (q.pop_now(s))
        {
            counter[s]++;
        }
    };

    std::unique_ptr<std::thread> produce_threads[PRODUCE_N];
    std::unique_ptr<std::thread> consumer_threads[CONSUMER_N];

    for (int i = 0; i < CONSUMER_N; i++)
    {
        consumer_threads[i].reset(new std::thread(consumer));
    }

    for (int i = 0; i < PRODUCE_N; i++)
    {
        produce_threads[i].reset(new std::thread(producer));
    }

    for (int i = 0; i < PRODUCE_N; i++)
    {
        produce_threads[i]->join();
    }
    for (int i = 0; i < CONSUMER_N; i++)
    {
        consumer_threads[i]->join();
    }

    bool has_race = false;
    for (int i = 0; i < 65536; i++)
    {
        if (counter[i] != MULTIPLIER * PRODUCE_N)
        {
            std::cout << "found race condition\t" << i << '\t' << counter[i] << std::endl;
            has_race = true;
            break;
        }
    }
    if (has_race)
    {
        std::cout << "found race condition" << std::endl;
    }
    else
    {
        std::cout << "no race condition" << std::endl;
    }
}