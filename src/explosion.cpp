#include "explosion.hpp"
#include "utility.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <unordered_set>

namespace sand {

auto explosion_ray(
    world& pixels,
    std::unordered_set<glm::ivec2>& checked,
    glm::ivec2 pos,
    glm::vec2 dir,
    const explosion& info
)
    -> void
{
    const auto a = pos;
    const auto b = a + glm::ivec2{(info.max_radius + info.scorch_radius) * dir};
    const auto steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    const auto blast_limit = random_from_range(info.min_radius, info.max_radius);
    auto scorch_limit = std::optional<float>{};

    for (int i = 0; i < steps; ++i) {
        const auto curr = a + (b - a) * i/steps;
        const auto curr_radius = glm::length(glm::vec2{curr} - glm::vec2{pos});

        if (!pixels.valid(curr)) {
            return;
        }

        if (scorch_limit.has_value()) {
            if (curr_radius >= *scorch_limit) {
                return;
            }
            if (properties(pixels.at(curr)).phase == pixel_phase::solid) {
                pixels.at(curr).colour *= 0.8f;
            }
            continue;
        }

        if (checked.contains(curr)) {
            continue;
        }

        using i2 = glm::ivec2;
        for (const auto offset : {i2{0, 0}, i2{0, 1}, i2{1, 0}, i2{1, 1}}) {
            if (!pixels.valid(curr + offset)) {
                return;
            }
            if (pixels.at(curr + offset).type == pixel_type::titanium) {
                scorch_limit = curr_radius + random_from_range(0.0f, info.scorch_radius);
                pixels.at(curr + offset).colour *= 0.8f;
                break;
            }
            if (curr_radius >= blast_limit) {
                scorch_limit = curr_radius + random_from_range(0.0f, info.scorch_radius);
                break;
            }
            if (scorch_limit.has_value()) {
                if (curr_radius >= *scorch_limit) {
                    return;
                }
                if (properties(pixels.at(curr)).phase == pixel_phase::solid) {
                    pixels.at(curr).colour *= 0.8f;
                }
                break;
            }

            pixels.set(curr + offset, random_unit() < 0.05f ? pixel::ember() : pixel::air());
            checked.emplace(curr);
        }
    }
}

auto apply_explosion(world& pixels, glm::ivec2 pos, const explosion& info) -> void
{
    std::unordered_set<glm::ivec2> checked;
    const auto boundary = info.max_radius + info.scorch_radius;

    for (int x = -boundary; x <= boundary; ++x) {
        const auto y = boundary;
        const auto dir = glm::normalize(glm::vec2{x, y});
        explosion_ray(pixels, checked, pos, dir, info);
    }

    for (int y = -boundary; y <= boundary; ++y) {
        const auto x = boundary;
        const auto dir = glm::normalize(glm::vec2{x, y});
        explosion_ray(pixels, checked, pos, dir, info);
    }

    for (int x = -boundary; x <= boundary; ++x) {
        const auto y = -boundary;
        const auto dir = glm::normalize(glm::vec2{x, y});
        explosion_ray(pixels, checked, pos, dir, info);
    }

    for (int y = -boundary; y <= boundary; ++y) {
        const auto x = -boundary;
        const auto dir = glm::normalize(glm::vec2{x, y});
        explosion_ray(pixels, checked, pos, dir, info);
    }
}

}