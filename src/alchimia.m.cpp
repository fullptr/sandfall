#include "window.h"
#include "log.h"
#include "shader.h"
#include "texture.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>

struct vertex
{
    glm::vec2 pos;
    glm::vec2 uv;

    vertex(glm::vec2 p, glm::vec2 u) : pos(p), uv(u) {}
};

std::uint32_t pos(std::uint32_t x, std::uint32_t y)
{
    return alc::texture::SIZE * y + x;
}

int main()
{
    using namespace alc;

    alc::window window("alchimia", 1280, 720);

    float size = 512.0f;
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        size, 0.0f, 1.0f, 0.0f,
        size, size, 1.0f, 1.0f,
        0.0f, size, 0.0f, 1.0f
    };

    unsigned int indices[] = {0, 1, 2, 0, 2, 3};

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Change to dealing with uint8_t (0 - 255)
    auto texture_data = std::make_unique<std::array<glm::vec4, alc::texture::SIZE * alc::texture::SIZE>>();
    for (size_t i = 0; i != alc::texture::SIZE; ++i) {
        for (size_t j = 0; j != alc::texture::SIZE; ++j) {
            (*texture_data)[pos(i, j)] = {(float)i / 256.0f, (float)j / 256.0f, 0.0, 1.0};
        }
    }
    (*texture_data)[pos(0, 0)] = {1.0, 1.0, 1.0, 1.0};
    (*texture_data)[pos(5, 0)] = {1.0, 1.0, 1.0, 1.0};
    (*texture_data)[pos(2, 3)] = {1.0, 1.0, 1.0, 1.0};


    alc::texture texture(*texture_data);
    //texture.set_buffer(*texture_data);

    alc::shader shader("res\\vertex.glsl", "res\\fragment.glsl");

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.bind();
    shader.load_sampler("u_texture", 0);
    shader.load_mat4("u_proj_matrix", glm::ortho(0.0f, window.width(), window.height(), 0.0f));
    texture.bind();
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    while (window.is_running()) {
        window.clear();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        window.swap_and_poll();
        
        auto mouse = window.get_mouse_pos();
        window.set_name(fmt::format("Mouse at ({}, {})", mouse.x, mouse.y));
    }
}