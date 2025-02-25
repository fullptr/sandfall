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

struct chunk
{
    bool    should_step      = true;
    bool    should_step_next = true;
    b2Body* triangles        = nullptr;
};

class level;
auto get_chunk_index(const level& w, glm::ivec2 chunk) -> std::size_t;
auto get_chunk_pos(const level& w, std::size_t index) -> glm::ivec2;

class world
{
    std::vector<pixel> d_pixels;
    std::size_t        d_width;
    std::size_t        d_height;

public:
    world(std::size_t width, std::size_t height, const std::vector<pixel>& pixels)
        : d_pixels{pixels}
        , d_width{width}
        , d_height{height}
    {
        assert(pixels.size() == width * height);
    }

    world(std::size_t width, std::size_t height)
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

    auto data() const -> const std::vector<pixel>& { return d_pixels; }
};

struct level
{
    b2World            physics;
    world        pixels;
    std::vector<chunk> chunks;
    glm::ivec2         spawn_point;
    player_controller  player;

    level(std::size_t width, std::size_t height);
    level(const level&) = delete;
    level& operator=(const level&) = delete;
    
    auto wake_chunk_with_pixel(glm::ivec2 pixel) -> void;
};

}