#include "common.hpp"
#include "world.hpp"
#include "mouse.hpp"
#include "window.hpp"
#include "camera.hpp"
#include "serialisation.hpp"
#include "utility.hpp"
#include "renderer.hpp"
#include "shape_renderer.hpp"
#include "enemy.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <memory>
#include <print>

auto main() -> int
{
    using namespace sand;

    auto window          = sand::window{"sandfall", 1280, 720};
    auto mouse           = sand::mouse{};
    auto keyboard        = sand::keyboard{};
    auto level           = sand::load_level("save4.bin");
    auto world_renderer  = sand::renderer{static_cast<u32>(level->pixels.width_in_pixels()), static_cast<u32>(level->pixels.height_in_pixels())};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};

    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = window.width(),
        .screen_height = window.height(),
        .world_to_screen = window.height() / 210.0f
    };

    while (window.is_running()) {
        const double dt = timer.on_update();
        window.begin_frame();
        mouse.on_new_frame();
        keyboard.on_new_frame();

        for (const auto event : window.events()) {
            mouse.on_event(event);
            keyboard.on_event(event);

            if (const auto e = event.get_if<sand::window_resize_event>()) {
                camera.screen_width = e->width;
                camera.screen_height = e->height;
                camera.world_to_screen = e->height / 210.0f;
            }
        }

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            accumulator -= sand::config::time_step;
            
            const auto desired_top_left = entity_centre(level->player) - sand::dimensions(camera) / (2.0f * camera.world_to_screen);
            if (desired_top_left != camera.top_left) {
                const auto diff = desired_top_left - camera.top_left;
                camera.top_left += 0.05f * diff;

                // Clamp the camera to the world, don't allow players to see the void
                const auto camera_dimensions_world_space = sand::dimensions(camera) / camera.world_to_screen;
                camera.top_left.x = std::clamp(camera.top_left.x, 0.0f, (float)level->pixels.width_in_pixels() - camera_dimensions_world_space.x);
                camera.top_left.y = std::clamp(camera.top_left.y, 0.0f, (float)level->pixels.height_in_pixels() - camera_dimensions_world_space.y);
            }
            
            updated = true;
            level->pixels.step();
        }

        update_entity(level->player, keyboard);
        for (auto& e : level->entities) {
            update_entity(e, keyboard);
        }

        world_renderer.bind();
        if (updated) {
            world_renderer.update(*level, camera);
        }
        world_renderer.draw();

        // TODO: Replace with actual sprite data
        shape_renderer.begin_frame(camera);      
        shape_renderer.draw_circle(entity_centre(level->player), {1.0, 1.0, 0.0, 1.0}, 3);
        for (const auto& e : level->entities) {
            shape_renderer.draw_circle(entity_centre(e), {0.5, 1.0, 0.5, 1.0}, 2.5);
        }  
        shape_renderer.end_frame();
        
        window.end_frame();
    }
    
    return 0;
}