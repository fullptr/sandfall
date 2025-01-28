#pragma once
#include <glm/glm.hpp>

namespace sand {
namespace config {

static constexpr auto time_step = 1.0f / 60.0f;
static constexpr auto gravity = glm::vec2{0.0f, 9.81f};

// Pixel Space
static constexpr int num_pixels = 256;
static constexpr int chunk_size = 64;
static_assert(num_pixels % chunk_size == 0);

// World Space
static constexpr int pixels_per_meter = 16;
static constexpr int world_width = num_pixels / pixels_per_meter;

}
}