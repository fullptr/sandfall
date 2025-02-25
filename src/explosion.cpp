#include "explosion.hpp"
#include "utility.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <unordered_set>

namespace sand {
namespace {

auto explosion_ray(level& w, glm::vec2 start, glm::vec2 end, const explosion& info) -> void
{
    // Calculate a step length small enough to hit every pixel on the path.
    const auto line = end - start;
    const auto step = line / glm::max(glm::abs(line.x), glm::abs(line.y));

    auto curr = start;

    const auto blast_limit = random_from_range(info.min_radius, info.max_radius);
    while (w.pixels.valid(curr) && glm::length2(curr - start) < glm::pow(blast_limit, 2)) {
        if (w.pixels[curr].type == pixel_type::titanium) {
            break;
        }
        w.pixels[curr] = random_unit() < 0.05f ? pixel::ember() : pixel::air();
        w.wake_chunk_with_pixel(curr);
        curr += step;
    }
    
    // Try to catch light to the first scorched pixel
    if (w.pixels.valid(curr)) {
        auto& pixel = w.pixels[curr];
        if (random_unit() < properties(pixel).flammability) {
            pixel.flags[is_burning] = true;
            w.wake_chunk_with_pixel(curr);
        }
    }

    const auto scorch_limit = glm::length(curr - start) + std::abs(random_normal(0.0f, info.scorch));
    while (w.pixels.valid(curr) && glm::length2(curr - start) < glm::pow(scorch_limit, 2)) {
        if (properties(w.pixels[curr]).phase == pixel_phase::solid) {
            w.pixels[curr].colour *= 0.8f;
            w.wake_chunk_with_pixel(curr);
        }
        curr += step;
    }
}

}

auto apply_explosion(level& pixels, glm::vec2 pos, const explosion& info) -> void
{
    const auto a = info.max_radius + 3 * info.scorch;
    for (int b = -a; b != a + 1; ++b) {
        explosion_ray(pixels, pos, pos + glm::vec2{b, a}, info);
        explosion_ray(pixels, pos, pos + glm::vec2{b, -a}, info);
        explosion_ray(pixels, pos, pos + glm::vec2{a, b}, info);
        explosion_ray(pixels, pos, pos + glm::vec2{-a, b}, info);
    }
}

}