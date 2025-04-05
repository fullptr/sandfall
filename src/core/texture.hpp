#pragma once
#include "common.hpp"

#include <cstdint>
#include <span>

#include <glm/glm.hpp>

namespace sand {

class texture
{
    u32 d_texture;
    u32 d_width;
    u32 d_height;

    texture(const texture&) = delete;
    texture& operator=(const texture&) = delete;

public:
    texture();
    texture(u32 width, u32 height);
    ~texture();

    auto set_data(std::span<const glm::vec4> data) -> void;
    auto bind() const -> void;

    auto resize(u32 width, u32 height) -> void;

    auto width() const -> u32 { return d_width; }
    auto height() const -> u32 { return d_height; }
};

}