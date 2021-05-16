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
#include <chrono>
#include <string_view>

constexpr glm::vec4 BACKGROUND = { 44.0f / 256.0f, 58.0f / 256.0f, 71.0f / 256.0f, 1.0 };

std::string_view to_string(alc::pixel_type type)
{
    switch (type) {
        case alc::pixel_type::air: return "air";
        case alc::pixel_type::sand: return "sand";
        case alc::pixel_type::water: return "water";
        case alc::pixel_type::rock: return "rock";
        case alc::pixel_type::red_sand: return "red_sand";
    }
}

class pixel_type_loop
{
    alc::pixel_type d_type = alc::pixel_type::air;

public:
    alc::pixel_type get() const { return d_type; }
    
    inline void operator++() {
        switch (d_type) {
            case alc::pixel_type::air: d_type = alc::pixel_type::sand; return;
            case alc::pixel_type::sand: d_type = alc::pixel_type::water; return;
            case alc::pixel_type::water: d_type = alc::pixel_type::rock; return;
            case alc::pixel_type::rock: d_type = alc::pixel_type::red_sand; return;
            case alc::pixel_type::red_sand: d_type = alc::pixel_type::air; return;
        }
    }

    inline void operator--() {
        switch (d_type) {
            case alc::pixel_type::air: d_type = alc::pixel_type::red_sand; return;
            case alc::pixel_type::sand: d_type = alc::pixel_type::air; return;
            case alc::pixel_type::water: d_type = alc::pixel_type::sand; return;
            case alc::pixel_type::rock: d_type = alc::pixel_type::water; return;
            case alc::pixel_type::red_sand: d_type = alc::pixel_type::rock; return;
        }
    }
};

int main()
{
    using namespace alc;

    alc::window window("alchimia", 1280, 720);

    float size = 720.0f;
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

    auto tile = std::make_unique<alc::tile>();
    tile->fill(pixel::air());

    pixel_type_loop loop;

    auto current_tool = pixel_type::sand;

    bool left_mouse_down = false; // TODO: Remove, do it in a better way

    window.set_callback([&](alc::event& event) {
        if (auto e = event.get_if<alc::mouse_pressed_event>()) {
            switch (e->button) {
                case 0: left_mouse_down = true; return;
            }
        }
        else if (auto e = event.get_if<alc::mouse_released_event>()) {
            switch (e->button) {
                case 0: left_mouse_down = false; return;
            }
        }
        else if (auto e = event.get_if<alc::mouse_scrolled_event>()) {
            if (e->y_offset > 0) {
                ++loop;
            } else if (e->y_offset < 0) {
                --loop;
            }
        }
    });

    alc::shader shader("res\\vertex.glsl", "res\\fragment.glsl");

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.bind();
    shader.load_sampler("u_texture", 0);
    shader.load_mat4("u_proj_matrix", glm::ortho(0.0f, window.width(), window.height(), 0.0f));
    tile->bind();
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    std::chrono::steady_clock clock;
    auto prev = clock.now();
    auto now = clock.now();
    double dt = 0;

    double frame_length = 1.0 / 60.0;
    double accumulator = 0;

    while (window.is_running()) {
        prev = now;
        now = clock.now();
        std::chrono::duration<double> dt_duration = (now - prev);
        double dt = dt_duration.count();

        accumulator += dt;
        while (accumulator > frame_length) {
            tile->simulate(frame_length);
            accumulator -= frame_length;
        }

        window.clear();
        if (left_mouse_down) {
            auto coord = glm::floor(((float)alc::tile::SIZE / (float)size) * window.get_mouse_pos());
            if (tile->valid(coord)) {
                switch (loop.get()) {
                    case alc::pixel_type::air: tile->set(coord, pixel::air()); break;
                    case alc::pixel_type::sand: tile->set(coord, pixel::sand()); break;
                    case alc::pixel_type::water: tile->set(coord, pixel::water()); break;
                    case alc::pixel_type::rock: tile->set(coord, pixel::rock()); break;
                    case alc::pixel_type::red_sand: tile->set(coord, pixel::red_sand()); break;
                }
            }
        }

        tile->update_texture();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        window.swap_and_poll();
        
        auto mouse = window.get_mouse_pos();
        window.set_name(fmt::format("Alchimia - Current tool: {}", to_string(loop.get())));
    }
}