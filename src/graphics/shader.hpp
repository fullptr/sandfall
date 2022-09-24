#pragma once
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <filesystem>

namespace sand {

std::string parse_shader(const std::string& filepath);

class shader
{
    std::uint32_t d_program;
    std::uint32_t d_vertex_shader;
    std::uint32_t d_fragment_shader;

    std::uint32_t get_location(const std::string& name) const;

public:
    shader(const std::filesystem::path& vertex_shader,
           const std::filesystem::path& fragment_shader);

    auto bind() const -> void;
    auto unbind() const -> void;

    auto load_mat4(const std::string& name, const glm::mat4& matrix) const -> void;
    auto load_int(const std::string& name, int value) const -> void;
    auto load_sampler(const std::string& name, int value) const -> void;
};

}