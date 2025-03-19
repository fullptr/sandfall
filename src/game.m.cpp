#include "world.hpp"
#include "pixel.hpp"
#include "config.hpp"
#include "utility.hpp"
#include "camera.hpp"
#include "update_rigid_bodies.hpp"
#include "explosion.hpp"
#include "mouse.hpp"
#include "player.hpp"
#include "world_save.hpp"
#include "renderer.hpp"
#include "shape_renderer.hpp"
#include "window.hpp"
#include "serialisation.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <memory>
#include <print>
#include <fstream>
#include <cmath>
#include <span>

auto main() -> int
{
    auto window          = sand::window{"sandfall", 1280, 720};
    auto mouse           = sand::mouse{};
    auto keyboard        = sand::keyboard{};
    auto level           = sand::new_level(4, 4);
    auto world_renderer  = sand::renderer{level->pixels.width(), level->pixels.height()};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};

    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = window.width(),
        .screen_height = window.height(),
        .world_to_screen = 720.0f / 256.0f
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
            }
            else if (const auto e = event.get_if<sand::mouse_scrolled_event>()) {
                const auto old_centre = mouse_pos_world_space(mouse, camera);
                camera.world_to_screen += 0.1f * e->offset.y;
                camera.world_to_screen = std::clamp(camera.world_to_screen, 1.0f, 100.0f);
                const auto new_centre = mouse_pos_world_space(mouse, camera);
                camera.top_left -= new_centre - old_centre;
            }
        }

        if (mouse.is_down(sand::mouse_button::right)) {
            camera.top_left -= mouse.offset() / camera.world_to_screen;
        }

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            accumulator -= sand::config::time_step;
            updated = true;
            level->pixels.step();
        }
        level->player.update(keyboard);

        const auto mouse_actual = mouse_pos_world_space(mouse, camera);
        const auto mouse_pixel = pixel_at_mouse(mouse, camera);

        // Render and display the world
        world_renderer.bind();
        if (updated) {
            world_renderer.update(*level, false, camera);
        }
        world_renderer.draw();

        shape_renderer.begin_frame(camera);

        // TODO: Replace with actual sprite data
        shape_renderer.draw_circle(level->player.centre(), {1.0, 1.0, 0.0, 1.0}, 3);

        shape_renderer.end_frame();
        

        window.end_frame();
    }
    
    return 0;
}