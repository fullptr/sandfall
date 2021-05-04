#ifndef INCLUDED_ALCHIMIA_SHADER
#define INCLUDED_ALCHIMIA_SHADER
#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace alc {

class shader
{
    std::uint32_t d_program;
    std::uint32_t d_vertex_shader;
    std::uint32_t d_fragment_shader;

    std::uint32_t get_location(const std::string& name) const;

public:
    shader(const std::string& vertex_shader, const std::string& fragment_shader);

    void bind() const;
    void unbind() const;

    void load_mat4(const std::string& name, const glm::mat4& matrix) const;
};

}

#endif // INCLUDED_ALCHIMIA_SHADER