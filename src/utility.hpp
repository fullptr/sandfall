#pragma once
#include <chrono>
#include <cstddef>
#include <string>
#include <span>
#include <format>
#include <filesystem>
#include <glm/glm.hpp>

namespace sand {

using i32 = std::int32_t;
using u32 = std::uint32_t;

static constexpr auto step = 1.0 / 60.0;

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
    auto on_update() -> double;
    auto frame_rate() const -> std::uint32_t { return d_frame_rate; }

    auto now() const -> clock::time_point;
};

auto random_from_range(float min, float max) -> float;
auto random_from_range(int min, int max) -> int;
auto random_from_circle(float radius) -> glm::ivec2;

auto random_normal(float centre, float sd) -> float;

template <typename Elements>
auto random_element(const Elements& elements)
{
    return elements[random_from_range(0, std::ssize(elements) - 1)];
}

auto coin_flip() -> bool;
auto sign_flip() -> int;
auto random_unit() -> float; // Same as random_from_range(0.0f, 1.0f)

auto _print_inner(const std::string& msg) -> void;

template <typename... Args>
auto print(std::string_view fmt, Args&&... args) -> void
{
    _print_inner(std::format(fmt, std::forward<Args>(args)...));
}

auto from_hex(int hex) -> glm::vec4;

auto get_executable_filepath() -> std::filesystem::path;

template <typename T>
auto lerp(const T& a, const T& b, float t) -> T
{
    return t * b + (1 - t) * a;
};

class window;
struct camera;
auto mouse_pos_world_space(const window& w, const camera& c) -> glm::vec2;
auto pixel_at_mouse(const window& w, const camera& c) -> glm::ivec2;

}