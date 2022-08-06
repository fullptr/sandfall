#include "tile.h"
#include "pixel.h"
#include "config.hpp"
#include "utility.hpp"
#include "editor.hpp"

#include "graphics/window.h"
#include "graphics/shader.h"
#include "graphics/texture.hpp"
#include "graphics/ui.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>

#include <cstddef>
#include <array>
#include <utility>
#include <fstream>
#include <memory>
#include <bitset>
#include <random>
#include <numbers>

using texture_data = std::array<glm::vec4, sand::tile_size * sand::tile_size>;

auto get_pos(glm::vec2 pos) -> std::size_t
{
    return pos.x + sand::tile_size * pos.y;
}

auto light_noise(glm::vec4 vec) -> glm::vec4
{
    return {
        std::clamp(vec.x + sand::random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        std::clamp(vec.y + sand::random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        std::clamp(vec.z + sand::random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        1.0f
    };
}

auto update_texture_data(texture_data& data, sand::tile& tile, bool show_chunks) -> void
{
    static const auto fire_colours = std::array{
        sand::from_hex(0xe55039),
        sand::from_hex(0xf6b93b),
        sand::from_hex(0xfad390)
    };

    for (std::size_t x = 0; x != sand::tile_size; ++x) {
        for (std::size_t y = 0; y != sand::tile_size; ++y) {
            const auto pos = get_pos({x, y});
            if (tile.at({x, y}).is_burning) {
                data[pos] = light_noise(sand::random_element(fire_colours));
            } else {
                data[pos] = tile.at({x, y}).colour;
            }

            if (show_chunks && tile.is_chunk_awake({x, y})) {
                data[pos].x += 0.05;
                data[pos].y += 0.05;
                data[pos].z += 0.05;
            }
        }
    }
}

auto main() -> int
{
    using namespace sand;

    auto window = sand::window{"sandfall", 1280, 720};

    float size = 720.0f;
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        size, 0.0f, 1.0f, 0.0f,
        size, size, 1.0f, 1.0f,
        0.0f, size, 0.0f, 1.0f
    };

    const std::uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    auto VAO = std::uint32_t{};
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    auto VBO = std::uint32_t{};
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    auto EBO = std::uint32_t{};
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    auto editor = sand::editor{};
    auto left_mouse_down = false; // TODO: Remove, do it in a better way

    auto ui = sand::ui{window};

    window.set_callback([&](sand::event& event) {
        auto& io = ImGui::GetIO();
        if (event.is_keyboard_event() && io.WantCaptureKeyboard) {
            return;
        }
        if (event.is_mount_event() && io.WantCaptureMouse) {
            return;
        }

        if (event.is<sand::mouse_pressed_event>()) {
            if (event.as<sand::mouse_pressed_event>().button == 0) { left_mouse_down = true; }
        }
        else if (event.is<sand::mouse_released_event>()) {
            if (event.as<sand::mouse_released_event>().button == 0) { left_mouse_down = false; }
        }
    });

    auto tile = std::make_unique<sand::tile>();
    auto shader = sand::shader{"res\\vertex.glsl", "res\\fragment.glsl"};
    auto texture = sand::texture{sand::tile_size, sand::tile_size};
    auto data = std::make_unique<texture_data>();

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.bind();
    shader.load_sampler("u_texture", 0);
    shader.load_mat4("u_proj_matrix", glm::ortho(0.0f, window.width(), window.height(), 0.0f));
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    auto accumulator = 0.0;
    auto timer = sand::timer{};
    auto show_demo = true;

    while (window.is_running()) {
        const double dt = timer.on_update();
        
        window.poll_events();
        window.clear();

        ui.begin_frame();
        display_ui(editor, *tile, timer);
        ui.end_frame();

        accumulator += dt;
        bool updated = false;
        while (accumulator > config::time_step) {
            tile->simulate(editor.show_chunks);
            accumulator -= config::time_step;
            updated = true;
        }

        if (updated) {
            update_texture_data(*data, *tile, editor.show_chunks);
            texture.set_data(*data);
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        
        if (left_mouse_down) {
            const auto mouse = glm::ivec2(((float)sand::tile_size / size) * window.get_mouse_pos());
            if (editor.brush_type == 0) {
                const auto coord = mouse + random_from_circle(editor.brush_size);
                if (tile->valid(coord)) {
                    tile->set(coord, editor.get_pixel());
                }
            }
            if (editor.brush_type == 1) {
                const auto half_extent = (int)(editor.brush_size / 2);
                for (int x = mouse.x - half_extent; x != mouse.x + half_extent; ++x) {
                    for (int y = mouse.y - half_extent; y != mouse.y + half_extent; ++y) {
                        if (tile->valid({x, y})) {
                            tile->set({x, y}, editor.get_pixel());
                        }
                    }
                }
            }
        }

        window.swap_buffers();
    }
    
    return 0;
}