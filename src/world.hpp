#pragma once
#include "pixel.hpp"
#include "serialise.hpp"
#include "config.hpp"

#include <cstdint>
#include <unordered_set>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <box2d/box2d.h>

namespace sand {

static constexpr int num_chunks = sand::config::num_pixels / sand::config::chunk_size;

struct chunk
{
    bool    should_step      = true;
    bool    should_step_next = true;
    b2Body* triangles        = nullptr;
};

auto get_chunk_index(glm::ivec2 chunk) -> std::size_t;
auto get_chunk_pos(std::size_t index) -> glm::ivec2;

class pixel_world
{
    std::vector<pixel> d_pixels;
    std::size_t        d_width;
    std::size_t        d_height;

public:
    pixel_world(std::size_t width, std::size_t height)
        : d_pixels{width * height, pixel::air()}
        , d_width{width}
        , d_height{height}
    {}

    auto valid(glm::ivec2 pos) const -> bool;
    auto operator[](glm::ivec2 pos) -> pixel&;
    auto operator[](glm::ivec2 pos) const -> const pixel&;

    inline auto begin() { return d_pixels.begin(); }
    inline auto end() { return d_pixels.end(); }

    inline auto width() const -> std::size_t { return d_width; }
    inline auto height() const -> std::size_t { return d_height; }

    auto serialise(auto& archive) -> void
    {
        archive(d_pixels, d_width, d_height);
    }
};

class world
{
    b2World            d_physics;
    pixel_world        d_pixels;
    std::vector<chunk> d_chunks;

public:
    world();

    // TODO: Remove
    auto physics() -> b2World& { return d_physics; }

    // Returns true if the given position exists and false otherwise
    auto valid(glm::ivec2 pos) const -> bool;

    auto set(glm::ivec2 pos, const pixel& p) -> void;
    auto fill(const pixel& p) -> void;

    auto at(glm::ivec2 pos) const -> const pixel&;
    auto at(glm::ivec2 pos) -> pixel&;
    auto type(glm::ivec2 pos) const -> pixel_type;

    auto new_frame() -> void;

    // Returns the rhs
    auto swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2;

    // Chunk API
    auto wake_chunk_with_pixel(glm::ivec2 pixel) -> void;
    auto wake_all_chunks() -> void;
    auto num_awake_chunks() const -> std::size_t;
    auto is_chunk_awake(glm::ivec2 pixel) const -> bool;

    auto get_chunks() const -> const std::vector<chunk>& { return d_chunks; }

    inline auto length() const { return sand::config::num_pixels; }

    auto serialise(auto& archive) -> void
    {
        archive(d_pixels);
    }
};

}