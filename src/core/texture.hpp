#pragma once
#include <cstdint>
#include <span>

#include <glm/glm.hpp>

namespace sand {

class texture
{
    std::uint32_t d_texture;
    std::uint32_t d_width;
    std::uint32_t d_height;

    texture(const texture&) = delete;
    texture& operator=(const texture&) = delete;

public:
    texture();
    texture(std::uint32_t width, std::uint32_t height);
    ~texture();

    auto set_data(std::span<const glm::vec4> data) -> void;
    auto bind() const -> void;

    auto resize(std::uint32_t width, std::uint32_t height) -> void;

    auto width() const -> std::uint32_t { return d_width; }
    auto height() const -> std::uint32_t { return d_height; }
};

}