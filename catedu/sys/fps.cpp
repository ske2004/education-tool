#include "fps.hpp"
#include "sokol/sokol_app.h"
#include <chrono>
#include <cmath>

void FpsCounter::update()
{
    count++;
    i++;
    i = i % 32;
    auto now = std::chrono::steady_clock::now();
    this->deltas[i] = std::chrono::duration_cast<std::chrono::microseconds>(
                          now - this->last_time)
                          .count() /
                      1000000.0;

    if (this->count > 32)
    {
        this->count = 32;
    }

    this->last_time = std::chrono::steady_clock::now();
}

double FpsCounter::get()
{
    double avg = this->deltas[0];

    for (int i = 1; i < this->count; i++)
    {
        avg += this->deltas[i];
    }

    return round(this->count / avg);
}
