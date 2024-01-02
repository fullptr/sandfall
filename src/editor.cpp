#include "editor.hpp"
#include "utility.hpp"
#include "camera.hpp"

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
    const camera& camera,
    const player& player
) -> bool
{
    auto updated = false;

    const auto mouse_actual = mouse_pos_world_space(window, camera);
    const auto mouse = pixel_at_mouse(window, camera);

    ImGui::ShowDemoWindow(&editor.show_demo);

    if (ImGui::Begin("Editor")) {
        ImGui::Text("Mouse");
        ImGui::Text("Position: {%.2f, %.2f}", mouse_actual.x, mouse_actual.y);
        ImGui::Text("Pixel: {%d, %d}", mouse.x, mouse.y);
        ImGui::Separator();

        ImGui::Text("Camera");
        ImGui::Text("Top Left: {%.2f, %.2f}", camera.top_left.x, camera.top_left.y);
        ImGui::Text("Screen width: %.2f", camera.screen_width);
        ImGui::Text("Screen height: %.2f", camera.screen_height);
        ImGui::Text("Scale: %f", camera.world_to_screen);
        ImGui::Separator();

        ImGui::Text("Player");
        ImGui::Text("Position: {%.2f, %.2f}", player.position.x, player.position.y);
        ImGui::Text("Velocity: {%.2f, %.2f}", player.velocity.x, player.velocity.y);
        ImGui::Text("Acceleration: {%.2f, %.2f}", player.acceleration.x, player.acceleration.y);
        ImGui::Separator();

        ImGui::Text("Info");
        ImGui::Text("FPS: %d", timer.frame_rate());
        ImGui::Text("Awake chunks: %d", world.num_awake_chunks());
        ImGui::Checkbox("Show chunks", &editor.show_chunks);
        if (ImGui::Button("Clear")) {
            world.wake_all_chunks();
            world.fill(sand::pixel::air());
        }
        ImGui::Separator();

        ImGui::Text("Brush");
        ImGui::SliderFloat("Size", &editor.brush_size, 0, 50);
        if (ImGui::RadioButton("Spray", editor.brush_type == 0)) editor.brush_type = 0;
        if (ImGui::RadioButton("Square", editor.brush_type == 1)) editor.brush_type = 1;
        if (ImGui::RadioButton("Explosion", editor.brush_type == 2)) editor.brush_type = 2;

        for (std::size_t i = 0; i != editor.pixel_makers.size(); ++i) {
            if (ImGui::Selectable(editor.pixel_makers[i].first.c_str(), editor.current == i)) {
                editor.current = i;
            }
        }
        ImGui::Separator();
        ImGui::Text("Levels");
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
                updated = true;
            }
            ImGui::SameLine();
            ImGui::Text("Save %d", i);
            ImGui::PopID();
        }
    }
    ImGui::End();

    return updated;
}
    
}