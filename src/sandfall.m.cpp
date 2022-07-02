#include "log.h"
#include "tile.h"
#include "pixel.h"
#include "world_settings.h"
#include "timer.hpp"
#include "random.hpp"

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

#include <cstdint>
#include <cstddef>
#include <array>
#include <utility>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <random>
#include <numbers>
#include <string_view>

constexpr glm::vec4 BACKGROUND = { 44.0f / 256.0f, 58.0f / 256.0f, 71.0f / 256.0f, 1.0 };

struct editor
{
    std::size_t current = 0;
    std::vector<std::pair<std::string, sand::pixel(*)()>> pixel_makers = {
        {"air", sand::pixel::air},
        {"sand", sand::pixel::sand},
        {"coal", sand::pixel::coal},
        {"water", sand::pixel::water},
        {"rock", sand::pixel::rock},
        {"red_sand", sand::pixel::red_sand},
    };
    
    auto get_pixel() -> sand::pixel
    {
        return pixel_makers[current].second();
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

    auto window = sand::window{"sandfall", 1280, 720};
    
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

    auto editor = ::editor{};
    auto left_mouse_down = false; // TODO: Remove, do it in a better way

    auto ui = sand::ui{window};

    window.set_callback([&](sand::event& event) {
        auto& io = ImGui::GetIO();
        if (event.is_keyboard_event() && io.WantCaptureKeyboard) {
            event.consume();
            return;
        }
        if (event.is_mount_event() && io.WantCaptureMouse) {
            event.consume();
            return;
        }

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
    });

    auto tile = std::make_unique<sand::tile>();
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
        window.poll_events();

        ui.begin_frame();

        bool show = true;
        ImGui::ShowDemoWindow(&show);

        if (ImGui::Begin("Editor")) {
            std::size_t i = 0;
            for (const auto& [name, _] : editor.pixel_makers) {
                char buf[32];
                sprintf(buf, name.c_str());
                if (ImGui::Selectable(buf, editor.current == i)) {
                    editor.current = i;
                }
                ++i;
            }
        }
        ImGui::End();

        accumulator += dt;
        bool updated = false;
        while (accumulator > frame_length) {
            tile->simulate(settings, frame_length);
            accumulator -= frame_length;
            updated = true;

        }

        if (updated) {
            texture.set_data(tile->data());
            window.set_name(fmt::format("Sandfall [FPS: {}]", timer.frame_rate()));
        }

        window.clear();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        
        if (left_mouse_down) {
            const auto coord = circle_offset(10.0f) + glm::ivec2((sand::tile_size_f / size) * window.get_mouse_pos());
            if (tile->valid(coord)) {
                tile->set(coord, editor.get_pixel());
            }
        }

        ui.end_frame();

        window.swap_buffers();
    }
}