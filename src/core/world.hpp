#pragma once
#include "common.hpp"
#include "pixel.hpp"
#include "serialise.hpp"
#include "world_save.hpp"
#include "entity.hpp"

#include <cstdint>
#include <unordered_set>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <box2d/box2d.h>

namespace sand {

auto get_chunk_top_left(chunk_pos pos) -> pixel_pos;

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
    i32                d_width;
    i32                d_height;
    
    auto at(pixel_pos pos) -> pixel&;
    auto at(chunk_pos pos) -> chunk&;

    auto wake_chunk(chunk_pos pos) -> void;
    
    public:
    world(i32 width, i32 height, const std::vector<pixel>& pixels)
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
    world(const world&) = delete;
    world(world&&) = delete;
    world& operator=(const world&) = delete;
    world& operator=(world&&) = delete;
    
    auto physics() -> b2World& { return d_physics; }
    
    auto step() -> void;

    auto wake_chunk_with_pixel(pixel_pos pixel) -> void;
    auto wake_all() -> void;
    
    auto is_valid_pixel(pixel_pos pos) const -> bool;
    auto is_valid_chunk(chunk_pos pos) const -> bool;
    auto set(pixel_pos pos, const pixel& p) -> void;
    auto swap(pixel_pos a, pixel_pos b) -> void;
    auto operator[](pixel_pos pos) const -> const pixel&;
    auto operator[](chunk_pos pos) const -> const chunk&;
    
    auto visit_no_wake(pixel_pos pos, auto&& updater) -> void
    {
        assert(is_valid_pixel(pos));
        updater(at(pos));
    }
    
    auto visit(pixel_pos pos, auto&& updater) -> void
    {
        visit_no_wake(pos, std::forward<decltype(updater)>(updater));
        wake_chunk_with_pixel(pos);
    }

    inline auto width_in_pixels() const -> i32 { return d_width; }
    inline auto height_in_pixels() const -> i32 { return d_height; }
    inline auto width_in_chunks() const -> i32 { return d_width / config::chunk_size; }
    inline auto height_in_chunks() const -> i32 { return d_height / config::chunk_size; }

    // Exposed for serialisation
    auto pixels() const -> const std::vector<pixel>& { return d_pixels; }
};

struct level
{
    world               pixels;
    pixel_pos           spawn_point;

    entity              player;
    std::vector<entity> entities;
    
    contact_listener    listener;

    level(i32 width, i32 height, const std::vector<pixel>& pixels, pixel_pos spawn);
};

}