#include "editor.hpp"

#include <cereal/archives/binary.hpp>
#include <imgui.h>

#include <fstream>
#include <format>

namespace sand {

auto display_ui(
    editor& editor,
    world& world,
    const timer& timer,
    const window& window,
    glm::ivec2 mouse
) -> void
{
    ImGui::ShowDemoWindow(&editor.show_demo);

    if (ImGui::Begin("Editor")) {
        for (std::size_t i = 0; i != editor.pixel_makers.size(); ++i) {
            if (ImGui::Selectable(editor.pixel_makers[i].first.c_str(), editor.current == i)) {
                editor.current = i;
            }
        }
        ImGui::SliderFloat("Brush size", &editor.brush_size, 0, 50);
        if (ImGui::Button("Clear")) {
            world.fill(sand::pixel::air());
        }

        ImGui::Text("FPS: %d", timer.frame_rate());
        ImGui::Text("Awake chunks: %d", world.num_awake_chunks());
        ImGui::Checkbox("Show chunks", &editor.show_chunks);

        for (int i = 0; i != 5; ++i) {
            ImGui::PushID(i);
            const auto filename = std::format("save{}.bin", i);
            if (ImGui::Button("Save")) {
                auto file = std::ofstream{filename, std::ios::binary};
                auto archive = cereal::BinaryOutputArchive{file};
                archive(world);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                auto file = std::ifstream{filename, std::ios::binary};
                auto archive = cereal::BinaryInputArchive{file};
                archive(world);
                world.wake_all_chunks();
            }
            ImGui::SameLine();
            ImGui::Text("Save %d", i);
            ImGui::PopID();
        }
        if (ImGui::RadioButton("Spray", editor.brush_type == 0)) {
            editor.brush_type = 0;
        }
        if (ImGui::RadioButton("Square", editor.brush_type == 1)) {
            editor.brush_type = 1;
        }
        if (ImGui::RadioButton("Explosion", editor.brush_type == 2)) {
            editor.brush_type = 2;
        }
        ImGui::Text("Brush: %d", editor.brush_type);

        if (world.valid(mouse)) {
            ImGui::Text("Power at mouse: %d", world.at(mouse).power);
        }
    }
    ImGui::End();
}
    
}