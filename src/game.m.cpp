#include "common.hpp"
#include "world.hpp"
#include "input.hpp"
#include "window.hpp"
#include "camera.hpp"
#include "serialisation.hpp"
#include "utility.hpp"
#include "renderer.hpp"
#include "debug.hpp"
#include "shape_renderer.hpp"
#include "ui.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <format>
#include <print>

enum class next_state
{
    main_menu,
    level,
    exit,
};

auto scene_main_menu(sand::window& window) -> next_state
{
    using namespace sand;
    auto timer = sand::timer{};
    auto ui    = sand::ui_engine{};

    constexpr auto clear_colour = from_hex(0x222f3e);

    while (window.is_running()) {
        const double dt = timer.on_update();
        window.begin_frame(clear_colour);

        for (const auto event : window.events()) {
            ui.on_event(event);
        }
        
        const auto scale = 3.0f;
        const auto button_width = 200;
        const auto button_height = 50;
        const auto button_left = (window.width() - button_width) / 2;

        if (ui.button("Start Game", {button_left, 100}, button_width, button_height, scale)) {
            std::print("loading level!\n");
            return next_state::level;
        }

        if (ui.button("Exit", {button_left, 160}, button_width, button_height, scale)) {
            std::print("exiting!\n");
            return next_state::exit;
        }

        if (ui.button("Enable Vsync", {10, 50}, button_width, button_height, scale)) {
            window.enable_vsync(true);
        }
        if (ui.button("Disable Vsync", {10, 110}, button_width, button_height, scale)) {
            window.enable_vsync(false);
        }

        const auto para_left = 100;
        const auto para_top = 300;
        ui.text("Lorem ipsum dolor sit amet, consectetur adipiscing elit,", {para_left, para_top}, scale);
        ui.text("sed do eiusmod tempor incididunt ut labore et dolore magna", {para_left, para_top + 1 * 11 * scale}, scale);
        ui.text("aliqua. Ut enim ad minim veniam, quis nostrud exercitation", {para_left, para_top + 2 * 11 * scale}, scale);
        ui.text("ullamco laboris nisi ut aliquip ex ea commodo consequat.", {para_left, para_top + 3 * 11 * scale}, scale);
        ui.text("Duis aute irure dolor in reprehenderit in voluptate velit", {para_left, para_top + 4 * 11 * scale}, scale);
        ui.text("esse cillum dolore eu fugiat nulla pariatur. Excepteur", {para_left, para_top + 5 * 11 * scale}, scale);
        ui.text("sint occaecat cupidatat non proident, sunt in culpa", {para_left, para_top + 6 * 11 * scale}, scale);
        ui.text("qui officia deserunt mollit anim id est laborum.", {para_left, para_top + 7 * 11 * scale}, scale);
        ui.text("ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz", {para_left, para_top + 8 * 11 * scale}, scale);
        ui.text("0123456789 () {} [] ^ < > - _ = + ! ? : ; . , @ % $ / \\ \" ' # ~ & | `", {para_left, para_top + 9 * 11 * scale}, scale);

        std::array<char, 8> buf = {};
        ui.text_box(sand::format_to(buf, "{}", timer.frame_rate()), {0, 0}, 120, 50, 3);
        ui.draw_frame(window.width(), window.height(), dt);
        window.end_frame();
    }

    return next_state::exit;
}

auto scene_level(sand::window& window) -> next_state
{
    using namespace sand;
    auto input           = sand::input{};
    auto level           = sand::load_level("save4.bin");
    auto world_renderer  = sand::renderer{level->pixels.width_in_pixels(), level->pixels.height_in_pixels()};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};
    auto debug_renderer  = sand::physics_debug_draw{&shape_renderer};
    auto ui              = sand::ui_engine{};

    const auto player_pos = glm::ivec2{ecs_entity_centre(level->entities, level->player) + glm::vec2{200, 0}};
    add_enemy(level->entities, level->pixels.physics(), pixel_pos::from_ivec2(player_pos));
    
    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = window.width(),
        .screen_height = window.height(),
        .world_to_screen = window.height() / 210.0f
    };
    
    while (window.is_running()) {
        const double dt = timer.on_update();
        window.begin_frame();
        input.on_new_frame();
        
        for (const auto event : window.events()) {
            if (const auto e = event.get_if<sand::keyboard_pressed_event>()) {
                if (e->key == keyboard::escape) {
                    return next_state::main_menu;
                }
            }
            else if (const auto e = event.get_if<sand::window_resize_event>()) {
                camera.screen_width = e->width;
                camera.screen_height = e->height;
                camera.world_to_screen = e->height / 210.0f;
            }

            input.on_event(event);
            ecs_on_event(level->entities, event);
        }
        
        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            accumulator -= sand::config::time_step;
            updated = true;
            ecs_on_update(level->entities, input);
            level->pixels.step();
        }
        
        const auto desired_top_left = ecs_entity_centre(level->entities, level->player) - sand::dimensions(camera) / (2.0f * camera.world_to_screen);
        if (desired_top_left != camera.top_left) {
            const auto diff = desired_top_left - camera.top_left;
            camera.top_left += (float)dt * 3 * diff;
            
            // Clamp the camera to the world, don't allow players to see the void
            const auto camera_dimensions_world_space = sand::dimensions(camera) / camera.world_to_screen;
            camera.top_left.x = std::clamp(camera.top_left.x, 0.0f, (float)level->pixels.width_in_pixels() - camera_dimensions_world_space.x);
            camera.top_left.y = std::clamp(camera.top_left.y, 0.0f, (float)level->pixels.height_in_pixels() - camera_dimensions_world_space.y);
        }
        
        if (updated) {
            world_renderer.update(*level);
        }
        world_renderer.draw(camera);
        
        // TODO: Replace with actual sprite data
        shape_renderer.begin_frame(camera);      
        shape_renderer.draw_circle(ecs_entity_centre(level->entities, level->player), {1.0, 1.0, 0.0, 1.0}, 3);
        for (auto e : level->entities.all()) {
            shape_renderer.draw_circle(ecs_entity_centre(level->entities, e), {0.5, 1.0, 0.5, 1.0}, 2.5);
        }

        const auto centre = ecs_entity_centre(level->entities, level->player);
        const auto direction = glm::normalize(mouse_pos_world_space(input, camera) - centre);
        shape_renderer.draw_line(centre, centre + 10.0f * direction, {1, 1, 1, 1}, 2);
        level->pixels.physics().SetDebugDraw(&debug_renderer);
        level->pixels.physics().DebugDraw();
        shape_renderer.end_frame();
        
        std::array<char, 8> buf = {};
        ui.text_box(sand::format_to(buf, "{}", timer.frame_rate()), {0, 0}, 120, 50, 3);
        ui.draw_frame(window.width(), window.height(), dt);

        window.end_frame();
    }

    return next_state::exit;
}

auto main() -> int
{
    using namespace sand;

    auto window = sand::window{"sandfall", 1280, 720};
    auto next   = next_state::main_menu;

    while (true) {
        switch (next) {
            case next_state::main_menu: {
                next = scene_main_menu(window);
            } break;
            case next_state::level: {
                next = scene_level(window);
            } break;
            case next_state::exit: {
                std::print("closing game\n");
                return 1;
            } break;
            default: {
                std::print("how did we get here\n");
                return 1; // TODO: Handle this better
            }
        }
    }
    
    return 0;
}