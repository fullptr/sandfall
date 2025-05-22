#pragma once
#include "common.hpp"
#include "pixel.hpp"
#include "serialise.hpp"
#include "world_save.hpp"
#include "entity.hpp"
#include "context.hpp"

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
};

class pixel_world
{
    std::vector<pixel> d_pixels;
    std::vector<chunk> d_chunks;
    i32                d_width;
    i32                d_height;
    
    auto at(pixel_pos pos) -> pixel&;
    auto at(chunk_pos pos) -> chunk&;

    auto wake_chunk(chunk_pos pos) -> void;
    
public:
    pixel_world(i32 width, i32 height, const std::vector<pixel>& pixels)
        : d_pixels{pixels}
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
    pixel_world(i32 width, i32 height)
        : pixel_world(width, height, std::vector<sand::pixel>(width * height, sand::pixel::air()))
    {}
    
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

struct physics_world
{
    b2WorldId world;
    std::unordered_map<chunk_pos, b2BodyId> chunk_bodies;

    physics_world(glm::vec2 gravity = config::gravity);
    ~physics_world();

    physics_world(const physics_world&) = delete;
    physics_world& operator=(const physics_world&) = delete;

    physics_world(physics_world&&) = default;
    physics_world& operator=(physics_world&&) = default;
};

struct level
{
    pixel_world   pixels;
    physics_world physics;
    registry      entities;

    pixel_pos     spawn_point;
    entity        player;
};

auto level_on_update(level& l, const context& ctx) -> void;
auto level_on_event(level& l, const context& ctx, const event& e) -> void;

}