#pragma once
#include <random>

namespace sand {

inline auto random_from_range(float min, float max) -> float
{
    static std::default_random_engine gen;
    return std::uniform_real_distribution(min, max)(gen);
}

inline auto random_from_range(int min, int max) -> int
{
    static std::default_random_engine gen;
    return std::uniform_int_distribution(min, max)(gen);
}

inline auto coin_flip() -> bool
{
    return random_from_range(0, 1) == 0;
}

inline auto sign_flip() -> int
{
    return coin_flip() ? 1 : -1;
}

}