#pragma once
#include <glm/glm.hpp>

namespace sand {

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

namespace config {

static constexpr auto time_step = 1.0f / 60.0f;
static constexpr auto gravity = glm::vec2{0.0f, 9.81f};

// Pixel Space
static constexpr i32 chunk_size = 64;

// World Space
static constexpr i32 pixels_per_meter = 16;

}

struct pixel_pos
{
    i32 x;
    i32 y;

    auto operator<=>(const pixel_pos&) const = default;
    explicit operator glm::ivec2() const { return {x, y}; }
};

inline auto operator+(pixel_pos pos, glm::ivec2 offset) -> pixel_pos
{
    return {pos.x + offset.x, pos.y + offset.y};
}

inline auto operator-(pixel_pos a, pixel_pos b) -> glm::ivec2
{
    return {a.x - b.x, a.y - b.y};
}

struct chunk_pos
{
    i32 x;
    i32 y;

    auto operator<=>(const chunk_pos&) const = default;
    explicit operator glm::ivec2() const { return {x, y}; }
};

}