#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cstddef>
#include <functional>
#include <span>

namespace sand {

namespace detail {

std::uint32_t new_vbo();
void delete_vbo(std::uint32_t vbo);
void bind_index_buffer(std::uint32_t vbo);
void set_data(std::uint32_t vbo, std::size_t size, const void* data);

}

template <typename T> 
class vertex_buffer
{
    std::uint32_t d_vbo;

    vertex_buffer(const vertex_buffer&) = delete;
    vertex_buffer& operator=(const vertex_buffer&) = delete;

public:
    vertex_buffer(std::span<const T> data) : d_vbo(detail::new_vbo()) { set_data(data); }
    vertex_buffer() : d_vbo(detail::new_vbo()) {}
    ~vertex_buffer() { detail::delete_vbo(d_vbo); }

    void bind() const {  T::set_buffer_attributes(d_vbo); }

    void set_data(std::span<const T> data)
    {
        detail::set_data(d_vbo, data.size_bytes(), data.data());
    }
};

}