#pragma once
#include "pixel.hpp"
#include "serialise.hpp"
#include "config.hpp"
#include "world_save.hpp"
#include "player.hpp"

#include <cstdint>
#include <unordered_set>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <box2d/box2d.h>

namespace sand {

auto get_chunk_pos(std::size_t width, std::size_t index) -> glm::ivec2;

struct chunk
{
    bool    should_step      = true;
    bool    should_step_next = true;
    b2Body* triangles        = nullptr;
};

class world
{
    b2World            d_physics;
    std::vector<pixel> d_pixels;
    std::vector<chunk> d_chunks;
    std::size_t        d_width;
    std::size_t        d_height;

    auto at(glm::ivec2) -> pixel&;

public:
    world(std::size_t width, std::size_t height, const std::vector<pixel>& pixels)
        : d_physics{{config::gravity.x, config::gravity.y}}
        , d_pixels{pixels}
        , d_width{width}
        , d_height{height}
    {
        assert(pixels.size() == width * height);
        assert(width % config::chunk_size == 0);
        assert(height % config::chunk_size == 0);
        const auto width_chunks = width / config::chunk_size;
        const auto height_chunks = height / config::chunk_size;
        d_chunks.resize(width_chunks * height_chunks);
    }

    auto step() -> void;

    auto physics() -> b2World& { return d_physics; }
    auto wake_chunk_with_pixel(glm::ivec2 pixel) -> void;
    auto wake_all() -> void;

    auto valid(glm::ivec2 pos) const -> bool;
    auto chunk_valid(glm::ivec2 pos) const -> bool;
    auto set(glm::ivec2 pos, const pixel& p) -> void;
    auto swap(glm::ivec2 a, glm::ivec2 b) -> void;
    auto operator[](glm::ivec2 pos) const -> const pixel&;

    auto visit(glm::ivec2 pos, auto&& updater) -> void
    {
        assert(valid(pos));
        auto& pixel = d_pixels[pos.x + d_width * pos.y];
        updater(pixel);
        wake_chunk_with_pixel(pos);
    }

    auto visit_no_wake(glm::ivec2 pos, auto&& updater) -> void
    {
        assert(valid(pos));
        auto& pixel = d_pixels[pos.x + d_width * pos.y];
        updater(pixel);
    }

    inline auto begin() { return d_pixels.begin(); }
    inline auto end() { return d_pixels.end(); }

    inline auto width() const -> std::size_t { return d_width; }
    inline auto height() const -> std::size_t { return d_height; }

    auto pixels() const -> const std::vector<pixel>& { return d_pixels; }
    auto chunks() const -> const std::vector<chunk>& { return d_chunks; }
};

struct level
{
    world              pixels;
    glm::ivec2         spawn_point;
    player_controller  player;

    level(std::size_t width, std::size_t height, const std::vector<pixel>& pixels);
    level(const level&) = delete;
    level& operator=(const level&) = delete;
};

}