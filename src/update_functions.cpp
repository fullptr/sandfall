#include "update_functions.h"

#include <array>
#include <utility>

#include <glm/glm.hpp>

namespace alc {

void update_sand(pixel_api&& api, double dt)
{
    auto& vel = api.get({0, 0}).velocity;
    vel += api.world_settings().gravity * (float)dt;
    glm::ivec2 offset{0, glm::max(1, (int)vel.y)};

    if (api.move_to(offset) != glm::ivec2{0, 0}) {
        return;
    }

    std::array<glm::ivec2, 2> offsets = {
        glm::ivec2{-1, 1}, glm::ivec2{1, 1},
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
    }

    for (auto offset : offsets) {
        if (api.move_to(offset) != glm::ivec2{0, 0}) {
            return;
        } else {
            api.get({0, 0}).velocity = {0.0, 0.0};
        }
    }
}

void update_water(pixel_api&& api, double dt)
{
    auto& vel = api.get({0, 0}).velocity;
    vel += api.world_settings().gravity * (float)dt;
    auto offset = glm::ivec2{0, glm::max(1, (int)vel.y)};
    
    if (api.move_to(offset) != glm::ivec2{0, 0}) {
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
        if (api.move_to(offset) != glm::ivec2{0, 0}) {
            if (offset.y == 0) {
                api.get(offset).velocity = {0.0, 0.0};
            }
            return;
        }
    }
}

void update_rock(pixel_api&& api, double dt)
{
    api.get({0, 0}).updated_this_frame = true;
}

}