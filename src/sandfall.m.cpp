#include "tile.h"
#include "pixel.h"
#include "config.hpp"
#include "utility.hpp"

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
#include <cereal/archives/binary.hpp>

#include <cstddef>
#include <array>
#include <utility>
#include <fstream>
#include <memory>
#include <bitset>
#include <random>
#include <numbers>

struct editor
{
    std::size_t current = 0;
    std::vector<std::pair<std::string, sand::pixel(*)()>> pixel_makers = {
        { "air",      sand::pixel::air      },
        { "sand",     sand::pixel::sand     },
        { "coal",     sand::pixel::coal     },
        { "dirt",     sand::pixel::dirt     },
        { "water",    sand::pixel::water    },
        { "lava",     sand::pixel::lava     },
        { "acid",     sand::pixel::acid     },
        { "rock",     sand::pixel::rock     },
        { "titanium", sand::pixel::titanium },
        { "steam",    sand::pixel::steam    },
        { "fuse",     sand::pixel::fuse     },
        { "ember",    sand::pixel::ember    },
        { "oil",      sand::pixel::oil      },
    };

    float brush_size = 5.0f;
    
    auto get_pixel() -> sand::pixel
    {
        return pixel_makers[current].second();
    }
};
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

    auto editor = ::editor{};
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
        ui.begin_frame();

        ImGui::ShowDemoWindow(&show_demo);

        if (ImGui::Begin("Editor")) {
            for (std::size_t i = 0; i != editor.pixel_makers.size(); ++i) {
                if (ImGui::Selectable(editor.pixel_makers[i].first.c_str(), editor.current == i)) {
                    editor.current = i;
                }
            }
            ImGui::SliderFloat("Brush size", &editor.brush_size, 0, 20);
            if (ImGui::Button("Clear")) {
                tile->fill(sand::pixel::air());
            }

            ImGui::Text("FPS: %d", timer.frame_rate());

            if (ImGui::Button("Save")) {
                auto file = std::ofstream{"save.bin", std::ios::binary};
                auto archive = cereal::BinaryOutputArchive{file};
                archive(*tile);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                auto file = std::ifstream{"save.bin", std::ios::binary};
                auto archive = cereal::BinaryInputArchive{file};
                archive(*tile);
            }
        }
        ImGui::End();

        accumulator += dt;
        bool updated = false;
        while (accumulator > config::time_step) {
            tile->simulate();
            accumulator -= config::time_step;
            updated = true;
        }

        if (updated) {
            texture.set_data(tile->data());
        }

        window.clear();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        
        if (left_mouse_down) {
            const auto coord = random_from_circle(editor.brush_size)
                             + glm::ivec2((sand::tile_size_f / size) * window.get_mouse_pos());
            if (tile->valid(coord)) {
                tile->set(coord, editor.get_pixel());
            }
        }

        ui.end_frame();
        window.swap_buffers();
    }
    
    return 0;
}