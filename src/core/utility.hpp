#pragma once
#include <chrono>
#include <concepts>
#include <cstddef>
#include <string>
#include <span>
#include <format>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/ext/vector_uint4_sized.hpp>
#include <box2d/box2d.h>

#include "common.hpp"

namespace sand {

static constexpr auto up = glm::ivec2{0, -1};
static constexpr auto right = glm::ivec2{1, 0};
static constexpr auto down = glm::ivec2{0, 1};
static constexpr auto left = glm::ivec2{-1, 0};
static constexpr auto offsets = {up, right, down, left};

static constexpr auto step = 1.0 / 60.0;

template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
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

// Colours are stored as 8-bit RGBA (glm::u8vec4). To do maths on one, convert it to
// floats in [0, 1] with to_float_colour, then back with to_byte_colour.
constexpr auto from_hex(int hex) -> glm::u8vec4
{
    return {
        static_cast<u8>((hex >> 16) & 0xff),
        static_cast<u8>((hex >> 8) & 0xff),
        static_cast<u8>(hex & 0xff),
        u8{0xff}
    };
}

inline auto to_float_colour(glm::u8vec4 colour) -> glm::vec4
{
    return glm::vec4(colour) / 255.0f;
}

// Clamps to [0, 1] and rounds to nearest
inline auto to_byte_colour(glm::vec4 colour) -> glm::u8vec4
{
    return glm::u8vec4(glm::clamp(colour, 0.0f, 1.0f) * 255.0f + 0.5f);
}

auto get_executable_filepath() -> std::filesystem::path;

// Same as glm::normalize, but returns the zero vector instead of NaN for zero-length input
inline auto normalize_or_zero(glm::vec2 v) -> glm::vec2
{
    return glm::dot(v, v) > 0.0f ? glm::normalize(v) : glm::vec2{0.0f, 0.0f};
}

template <typename T>
auto lerp(const T& a, const T& b, float t) -> T
{
    return t * b + (1 - t) * a;
};

inline auto lerp(glm::u8vec4 a, glm::u8vec4 b, float t) -> glm::u8vec4
{
    return to_byte_colour(lerp(to_float_colour(a), to_float_colour(b), t));
}

struct camera;
class input;
auto mouse_pos_world_space(const input& in, const camera& c) -> glm::vec2;
auto pixel_at_mouse(const input& in, const camera& c) -> pixel_pos;

auto pixel_to_physics(pixel_pos px) -> b2Vec2;
auto pixel_to_physics(glm::vec2 px) -> b2Vec2;
auto pixel_to_physics(float px) -> float;

// Converts a point in world space to pixel space
auto physics_to_pixel(b2Vec2 px) -> glm::vec2;
auto physics_to_pixel(float px) -> float;

inline auto to_string(glm::ivec2 v) -> std::string
{
    return std::format("glm::ivec2{{{}, {}}}", v.x, v.y);
}

inline auto to_string(glm::vec2 v) -> std::string
{
    return std::format("glm::vec2{{{}, {}}}", v.x, v.y);
}

template <typename... Args>
auto format_to(std::span<char> buffer, std::format_string<Args...> fmt, Args&&... args) -> std::string_view
{
    const auto result = std::format_to_n(buffer.data(), buffer.size(), fmt, std::forward<Args>(args)...);
    return {buffer.data(), result.out};
}

template <typename T>
concept has_to_string_member = requires(T obj)
{
    { obj.to_string() } -> std::convertible_to<std::string>;
};

template <has_to_string_member T>
struct std::formatter<T> : std::formatter<std::string>
{
    auto format(const T& obj, auto& ctx) const {
        return std::formatter<std::string>::format(obj.to_string(), ctx);
    }
};

template <typename T>
concept has_to_string_free_function = requires(T obj)
{
    { sand::to_string(obj) } -> std::convertible_to<std::string>;
};

template <has_to_string_free_function T>
struct std::formatter<T> : std::formatter<std::string>
{
    auto format(const T& obj, auto& ctx) const {
        return std::formatter<std::string>::format(sand::to_string(obj), ctx);
    }
};

template <typename T>
const T& signed_index(const std::vector<T>& v, int index)
{
    while (index < 0) index += v.size();
    while (index >= v.size()) index -= v.size();
    return v[index];
}

}