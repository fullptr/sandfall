#include "window.h"
#include "log.h"
#include "shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <cstddef>

struct vertex
{
    glm::vec2 pos;
};

int main()
{
    using namespace alc;

    alc::window window("alchimia", 1280, 720);

    std::uint32_t vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    std::uint32_t vbo, ebo;

    vertex quad[4] = { {{0.0, 0.0}}, {{0.5, 0.0}}, {{0.0, 0.5}}, {{0.5, 0.5}} };
    glCreateBuffers(1, &vbo);
    glNamedBufferData(vbo, sizeof(vertex) * 4, quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    std::uint32_t indices[6] = {0, 1, 2, 1, 2, 3};
    glCreateBuffers(1, &ebo);
    glNamedBufferData(ebo, sizeof(std::uint32_t) * 6, indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec2), (void*)0
    );
    glEnableVertexAttribArray(0);

    alc::shader shader("res/vertex.glsl", "res/fragment.glsl");
    shader.bind();
    shader.load_mat4("u_proj_matrix", glm::ortho(0.0, 1280.0, 720.0, 0.0));

    while (window.is_running()) {
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        window.on_update(0.0);
        auto mouse = window.get_mouse_pos();
        window.set_name(fmt::format("Mouse at ({}, {})", mouse.x, mouse.y));
    }

    log::info("Cleaning up\n");
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
}