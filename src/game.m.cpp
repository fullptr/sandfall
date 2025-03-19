#include "config.hpp"
#include "world.hpp"
#include "mouse.hpp"
#include "window.hpp"
#include "camera.hpp"
#include "serialisation.hpp"
#include "utility.hpp"
#include "renderer.hpp"
#include "shape_renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <memory>
#include <print>

auto main() -> int
{
    auto window          = sand::window{"sandfall", 1280, 720};
    auto mouse           = sand::mouse{};
    auto keyboard        = sand::keyboard{};
    auto level           = sand::load_level("save0.bin");
    auto world_renderer  = sand::renderer{level->pixels.width(), level->pixels.height()};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};

    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = window.width(),
        .screen_height = window.height(),
        .world_to_screen = window.height() / 128.0f
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
                camera.world_to_screen = e->height / 128.0f;
            }
        }

        const auto desired_top_left = level->player.centre() - sand::dimensions(camera) / (2.0f * camera.world_to_screen);
        if (desired_top_left != camera.top_left) {
            const auto diff = desired_top_left - camera.top_left;
            camera.top_left += 0.05f * diff;
        }

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            accumulator -= sand::config::time_step;
            updated = true;
            level->pixels.step();
        }
        level->player.update(keyboard);

        world_renderer.bind();
        if (updated) {
            world_renderer.update(*level, false, camera);
        }
        world_renderer.draw();

        // TODO: Replace with actual sprite data
        shape_renderer.begin_frame(camera);        
        shape_renderer.draw_circle(level->player.centre(), {1.0, 1.0, 0.0, 1.0}, 3);
        shape_renderer.end_frame();
        
        window.end_frame();
    }
    
    return 0;
}