#include "graphics/buffer.hpp"

#include <glad/glad.h>

#include <ranges>

namespace sand {

namespace detail {

std::uint32_t new_vbo()
{
    std::uint32_t vbo = 0;
    glCreateBuffers(1, &vbo);
    return vbo;
}

void delete_vbo(std::uint32_t vbo)
{
    glDeleteBuffers(1, &vbo);
}

void bind_index_buffer(std::uint32_t vbo)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
}

void set_data(std::uint32_t vbo, std::size_t size, const void* data)
{
    glNamedBufferData(vbo, size, data, GL_STATIC_DRAW);
}

}

}