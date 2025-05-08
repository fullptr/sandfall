#include "explosion.hpp"
#include "utility.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <unordered_set>

namespace sand {
namespace {

auto nearest_pixel(glm::vec2 pos) -> pixel_pos
{
    return {static_cast<int>(pos.x), static_cast<int>(pos.y)};
}

auto explosion_ray(pixel_world& w, glm::vec2 start, glm::vec2 end, const explosion& info) -> void
{
    // Calculate a step length small enough to hit every pixel on the path.
    const auto line = end - start;
    const auto step = line / glm::max(glm::abs(line.x), glm::abs(line.y));

    auto curr = start;

    const auto blast_limit = random_from_range(info.min_radius, info.max_radius);
    while (w.is_valid_pixel(nearest_pixel(curr)) && glm::length2(curr - start) < glm::pow(blast_limit, 2)) {
        if (w[nearest_pixel(curr)].type == pixel_type::titanium) {
            break;
        }
        w.set(nearest_pixel(curr), random_unit() < 0.05f ? pixel::ember() : pixel::air());
        curr += step;
    }
    
    // Try to catch light to the first scorched pixel
    if (w.is_valid_pixel(nearest_pixel(curr))) {
        if (random_unit() < properties(w[nearest_pixel(curr)]).flammability) {
            w.visit(nearest_pixel(curr), [&](pixel& p) { p.flags[is_burning] = true; });
        }
    }

    const auto scorch_limit = glm::length(curr - start) + std::abs(random_normal(0.0f, info.scorch));
    while (w.is_valid_pixel(nearest_pixel(curr)) && glm::length2(curr - start) < glm::pow(scorch_limit, 2)) {
        if (properties(w[nearest_pixel(curr)]).phase == pixel_phase::solid) {
            w.visit(nearest_pixel(curr), [&](pixel& p) { p.colour *= 0.8f; });
        }
        curr += step;
    }
}

}

auto apply_explosion(pixel_world& w, pixel_pos pos, const explosion& info) -> void
{
    const auto p = glm::vec2{pos.x, pos.y};
    const auto a = info.max_radius + 3 * info.scorch;
    for (int b = -a; b != a + 1; ++b) {
        explosion_ray(w, p, p + glm::vec2{b, a}, info);
        explosion_ray(w, p, p + glm::vec2{b, -a}, info);
        explosion_ray(w, p, p + glm::vec2{a, b}, info);
        explosion_ray(w, p, p + glm::vec2{-a, b}, info);
    }
}

}