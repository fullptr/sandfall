#pragma once
#include "common.hpp"

#include <cstdint>
#include <span>

#include <glm/glm.hpp>

namespace sand {

enum class texture_type
{
    rgba,
    red,
};

class texture
{
    texture_type d_type;

    u32 d_texture = 0;
    u32 d_width   = 0;
    u32 d_height  = 0;

    texture(const texture&) = delete;
    texture& operator=(const texture&) = delete;

public:
    texture(texture_type type = texture_type::rgba);
    texture(u32 width, u32 height, texture_type type = texture_type::rgba);
    texture(const unsigned char* data, i32 width, i32 height);
    ~texture();

    auto set_data(std::span<const glm::vec4> data) -> void;
    auto set_data(std::span<const unsigned char> data, i32 width, i32 height) -> void;
    auto set_data(const unsigned char* data, i32 width, i32 height) -> void;

    auto set_subdata(std::span<const unsigned char> data, glm::ivec2 top_left, i32 width, i32 height) -> void;
    auto bind() const -> void;

    auto resize(u32 width, u32 height) -> void;

    auto width() const -> u32 { return d_width; }
    auto height() const -> u32 { return d_height; }
};

}