#include "window.h"
#include "log.h"
#include "shader.h"
#include "tile.h"
#include "pixel.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>

constexpr glm::vec4 BACKGROUND = { 44.0f / 256.0f, 58.0f / 256.0f, 71.0f / 256.0f, 1.0 };

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

    alc::tile tile;
    tile.fill(pixel::air());

    bool left_mouse_down = false; // TODO: Remove, do it in a better way
    bool right_mouse_down = false;

    window.set_callback([&](alc::event& event) {
        if (auto e = event.get_if<alc::mouse_pressed_event>()) {
            switch (e->button) {
                case 0: left_mouse_down = true; return;
                case 1: right_mouse_down = true; return;
            }
        }
        else if (auto e = event.get_if<alc::mouse_released_event>()) {
            switch (e->button) {
                case 0: left_mouse_down = false; return;
                case 1: right_mouse_down = false; return;
            }
        }
    });

    alc::shader shader("res\\vertex.glsl", "res\\fragment.glsl");

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.bind();
    shader.load_sampler("u_texture", 0);
    shader.load_mat4("u_proj_matrix", glm::ortho(0.0f, window.width(), window.height(), 0.0f));
    tile.bind();
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    while (window.is_running()) {
        window.clear();
        tile.simulate();
        if (left_mouse_down) {
            auto coord = glm::floor(((float)alc::tile::SIZE / size) * window.get_mouse_pos());
            tile.set(coord, pixel::rock());
        } else if (right_mouse_down) {
            auto coord = glm::floor(((float)alc::tile::SIZE / size) * window.get_mouse_pos());
            tile.set(coord, pixel::water());
        }
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        window.swap_and_poll();
        
        auto mouse = window.get_mouse_pos();
        window.set_name(fmt::format("Mouse at ({}, {})", mouse.x, mouse.y));
    }
}