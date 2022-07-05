#include "utility.hpp"

#include <random>
#include <iostream>

namespace sand {

timer::timer()
    : d_clock()
    , d_prev_time(d_clock.now())
    , d_curr_time(d_prev_time)
    , d_last_time_printed(d_prev_time)
    , d_frame_count(0)
{}

double timer::on_update()
{
    d_prev_time = d_curr_time;
    d_curr_time = d_clock.now();
    ++d_frame_count;

    if (d_curr_time - d_last_time_printed >= std::chrono::seconds(1)) {
        d_frame_rate = d_frame_count;
        d_frame_count = 0;
        d_last_time_printed = d_curr_time;
    }

    std::chrono::duration<double> dt = d_curr_time - d_prev_time;
    return dt.count();
}

auto random_from_range(float min, float max) -> float
{
    static std::default_random_engine gen;
    return std::uniform_real_distribution(min, max)(gen);
}

auto random_from_range(int min, int max) -> int
{
    static std::default_random_engine gen;
    return std::uniform_int_distribution(min, max)(gen);
}

auto coin_flip() -> bool
{
    return random_from_range(0, 1) == 0;
}

auto sign_flip() -> int
{
    return coin_flip() ? 1 : -1;
}

auto _print_inner(const std::string& msg) -> void
{
    std::cout << msg;
}
    
}