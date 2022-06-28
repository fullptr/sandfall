#include "log.h"
#include "tile.h"
#include "pixel.h"
#include "world_settings.h"
#include "timer.hpp"

#include "graphics/window.h"
#include "graphics/shader.h"
#include "graphics/texture.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <cstddef>
#include <array>
#include <utility>
#include <memory>
#include <chrono>
#include <random>
#include <numbers>
#include <string_view>

constexpr glm::vec4 BACKGROUND = { 44.0f / 256.0f, 58.0f / 256.0f, 71.0f / 256.0f, 1.0 };

struct pixel_type_loop
{
    int type = 0;

    auto get_pixel() -> sand::pixel
    {
        switch (type) {
            case 0: return sand::pixel::air();
            case 1: return sand::pixel::sand();
            case 2: return sand::pixel::water();
            case 3: return sand::pixel::rock();
            case 4: return sand::pixel::red_sand();
            default: return sand::pixel::air();
        }
    }

    auto get_pixel_name() -> std::string_view
    {
        switch (type) {
            case 0: return "air";
            case 1: return "sand";
            case 2: return "water";
            case 3: return "rock";
            case 4: return "red_sand";
            default: return "unknown";
        }
    }
};

float random_from_range(float min, float max)
{
    static std::default_random_engine gen;
    return std::uniform_real_distribution(min, max)(gen);
}

auto circle_offset(float radius) -> glm::ivec2
{
    const auto r = random_from_range(0, radius);
    const auto theta = random_from_range(0, 2 * std::numbers::pi);
    return { r * std::cos(theta), r * std::sin(theta) };
}

int main()
{
    using namespace sand;

    auto window = sand::window{"alchimia", 1280, 720};

    auto settings = sand::world_settings{
        .gravity = {0.0f, 9.81f}
    };

    float size = 720.0f;
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        size, 0.0f, 1.0f, 0.0f,
        size, size, 1.0f, 1.0f,
        0.0f, size, 0.0f, 1.0f
    };

    std::uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    std::uint32_t VAO = 0;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    std::uint32_t VBO = 0;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    std::uint32_t EBO = 0;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    auto tile = std::make_unique<sand::tile>();

    pixel_type_loop loop;

    bool left_mouse_down = false; // TODO: Remove, do it in a better way

    window.set_callback([&](sand::event& event) {
        if (auto e = event.get_if<sand::mouse_pressed_event>()) {
            switch (e->button) {
                case 0: left_mouse_down = true; return;
            }
        }
        else if (auto e = event.get_if<sand::mouse_released_event>()) {
            switch (e->button) {
                case 0: left_mouse_down = false; return;
            }
        }
        else if (auto e = event.get_if<sand::mouse_scrolled_event>()) {
            if (e->y_offset > 0) {
                ++loop.type;
            } else if (e->y_offset < 0) {
                --loop.type;
            }
        }
    });

    auto shader = sand::shader{"res\\vertex.glsl", "res\\fragment.glsl"};
    auto texture = sand::texture{sand::tile_size, sand::tile_size};

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.bind();
    shader.load_sampler("u_texture", 0);
    shader.load_mat4("u_proj_matrix", glm::ortho(0.0f, window.width(), window.height(), 0.0f));
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    auto frame_length = 1.0 / 60.0;
    auto accumulator = 0.0;
    auto timer = sand::timer{};

    while (window.is_running()) {
        const double dt = timer.on_update();

        accumulator += dt;
        bool updated = false;
        while (accumulator > frame_length) {
            tile->simulate(settings, frame_length);
            accumulator -= frame_length;
            updated = true;
        }

        if (updated) {
            window.clear();
            texture.set_data(tile->data());
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            window.swap_buffers();
        }
        
        if (left_mouse_down) {
            auto coord = circle_offset(10.0f) + glm::ivec2((sand::tile_size_f / size) * window.get_mouse_pos());
            if (tile->valid(coord)) {
                tile->set(coord, loop.get_pixel());
            }
        }

        window.poll_events();
        window.set_name(fmt::format("Alchimia - Current tool: {} [FPS: {}]", loop.get_pixel_name(), timer.frame_rate()));
    }
}