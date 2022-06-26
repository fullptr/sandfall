#include "update_functions.h"

#include <array>
#include <utility>

#include <glm/glm.hpp>

namespace sand {
namespace {

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + tile::SIZE * pos.y;
}

}


void update_sand(pixel_api&& api, tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    auto& vel = pixels[get_pos(pos)].velocity;
    vel += settings.gravity * (float)dt;
    glm::ivec2 offset{0, glm::max(1, (int)vel.y)};

    if (api.move_to(offset)) {
        return;
    }

    std::array<glm::ivec2, 2> offsets = {
        glm::ivec2{-1, 1}, glm::ivec2{1, 1},
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
    }

    for (auto offset : offsets) {
        if (api.move_to(offset)) {
            return;
        } else {
            pixels[get_pos(pos)].velocity = {0.0, 0.0};
        }
    }
}

void update_water(pixel_api&& api, tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    auto& vel = pixels[get_pos(pos)].velocity;
    vel += settings.gravity * (float)dt;
    auto offset = glm::ivec2{0, glm::max(1, (int)vel.y)};
    
    if (api.move_to(offset)) {
        return;
    }

    std::array<glm::ivec2, 4> offsets = {
        glm::ivec2{-1, 1}, glm::ivec2{1, 1}, glm::ivec2{-1, 0}, glm::ivec2{1, 0}
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
        std::swap(offsets[2], offsets[3]);
    }

    for (auto offset : offsets) {
        if (api.move_to(offset)) {
            if (offset.y == 0) {
                pixels[get_pos(pos + offset)].velocity = {0.0, 0.0};
            }
            return;
        }
    }
}

void update_rock(pixel_api&& api, tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    pixels[get_pos(pos)].updated_this_frame = true;
}

}