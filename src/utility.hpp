#pragma once
#include <chrono>
#include <cstddef>
#include <string>
#include <glm/glm.hpp>

namespace sand {

template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template <typename Func>
class scope_exit
{
    Func d_func;

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;

    scope_exit(scope_exit&&) = delete;
    scope_exit& operator=(scope_exit&&) = delete;

public:
    scope_exit(Func&& func) : d_func(std::move(func)) {}
    ~scope_exit() { d_func(); }
};

class timer
{
    using clock = std::chrono::steady_clock;

    clock d_clock;
    clock::time_point d_prev_time;
    clock::time_point d_curr_time;
    clock::time_point d_last_time_printed;
    
    std::uint32_t d_frame_count;
    std::uint32_t d_frame_rate = 0;

public:
    timer();
    double on_update();
    std::uint32_t frame_rate() const { return d_frame_rate; }
};

auto random_from_range(float min, float max) -> float;
auto random_from_range(int min, int max) -> int;
auto random_from_circle(float radius) -> glm::ivec2;
auto coin_flip() -> bool;
auto sign_flip() -> int;

auto _print_inner(const std::string& msg) -> void;

template <typename... Args>
auto print(std::string_view fmt, Args&&... args) -> void
{
    _print_inner(std::format(fmt, std::forward<Args>(args)...));
}

}