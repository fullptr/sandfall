#include "explosion.hpp"
#include "utility.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <unordered_set>

namespace sand {

auto explosion_ray(
    world& pixels,
    glm::vec2 start,
    glm::vec2 end,
    const explosion& info
)
    -> void
{
    // Calculate a step length small enough to hit every pixel on the path.
    const auto steps = glm::max(glm::abs(start.x - end.x), glm::abs(start.y - end.y));
    const auto step = (end - start) / steps;

    auto curr = start;

    const auto blast_limit = random_from_range(info.min_radius, info.max_radius);
    while (glm::length(curr - start) < blast_limit) {
        if (!pixels.valid(curr)) return;
        if (pixels.at(curr).type == pixel_type::titanium) {
            break;
        }
        pixels.set(curr, random_unit() < 0.05f ? pixel::ember() : pixel::air());
        curr += step;
    }
    
    const auto scorch_limit = glm::length(curr - start) + std::abs(random_normal(0.0f, info.scorch));
    while (glm::length(curr - start) < scorch_limit) {
        if (!pixels.valid(curr)) return;
        if (properties(pixels.at(curr)).phase == pixel_phase::solid) {
            pixels.at(curr).colour *= 0.8f;
        }
        curr += step;
    }
}

auto apply_explosion(world& pixels, glm::vec2 pos, const explosion& info) -> void
{
    const auto boundary = info.max_radius + 3 * info.scorch;

    for (int x = -boundary; x <= boundary; ++x) {
        const auto y = boundary;
        explosion_ray(pixels, pos, pos + glm::vec2{x, y}, info);
    }

    for (int y = -boundary; y <= boundary; ++y) {
        const auto x = boundary;
        explosion_ray(pixels, pos, pos + glm::vec2{x, y}, info);
    }

    for (int x = -boundary; x <= boundary; ++x) {
        const auto y = -boundary;
        explosion_ray(pixels, pos, pos + glm::vec2{x, y}, info);
    }

    for (int y = -boundary; y <= boundary; ++y) {
        const auto x = -boundary;
        explosion_ray(pixels, pos, pos + glm::vec2{x, y}, info);
    }
}

}