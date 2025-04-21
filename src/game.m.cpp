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

#include <memory>
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
    auto timer           = sand::timer{};
    auto ui              = sand::ui_engine{};

    constexpr auto clear_colour = from_hex(0x3d3d3d);

    while (window.is_running()) {
        const double dt = timer.on_update();
        window.begin_frame(clear_colour);

        for (const auto event : window.events()) {
            ui.on_event(event);
        }
        
        const auto scale = 3.0f;

        ui.text("Start Game", {215, 200}, scale);
        if (ui.button("button1", {100, 100}, 100, 100)) {
            std::print("loading level!\n");
            return next_state::level;
        }

        ui.text("Exit", {215, 350}, scale);
        if (ui.button("button2", {100, 250}, 100, 100)) {
            std::print("exiting!\n");
            return next_state::exit;
        }

        ui.text("Lorem ipsum dolor sit amet, consectetur adipiscing elit,", {500, 200}, scale);
        ui.text("sed do eiusmod tempor incididunt ut labore et dolore magna", {500, 200 + 1 * 11 * scale}, scale);
        ui.text("aliqua. Ut enim ad minim veniam, quis nostrud exercitation", {500, 200 + 2 * 11 * scale}, scale);
        ui.text("ullamco laboris nisi ut aliquip ex ea commodo consequat.", {500, 200 + 3 * 11 * scale}, scale);
        ui.text("Duis aute irure dolor in reprehenderit in voluptate velit", {500, 200 + 4 * 11 * scale}, scale);
        ui.text("esse cillum dolore eu fugiat nulla pariatur. Excepteur", {500, 200 + 5 * 11 * scale}, scale);
        ui.text("sint occaecat cupidatat non proident, sunt in culpa", {500, 200 + 6 * 11 * scale}, scale);
        ui.text("qui officia deserunt mollit anim id est laborum.", {500, 200 + 7 * 11 * scale}, scale);

        ui.text("ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz", {100, 600}, scale);
        ui.text("0123456789 () {} [] ^ < > - _ = + ! ? : ; . , @ % $ / \\ \" ' # ~ &", {100, 600 + 1 * 11 * scale}, scale);

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

    const auto player_pos = glm::ivec2{entity_centre(level->player) + glm::vec2{200, 0}};
    auto other_entity = make_enemy(level->pixels.physics(), pixel_pos::from_ivec2(player_pos));
    level->entities.push_back(other_entity);

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
            input.on_event(event);

            if (const auto e = event.get_if<sand::window_resize_event>()) {
                camera.screen_width = e->width;
                camera.screen_height = e->height;
                camera.world_to_screen = e->height / 210.0f;
            }
        }

        if (input.is_down_this_frame(keyboard::escape)) {
            return next_state::main_menu;
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

        update_entity(level->player, input);
        for (auto& e : level->entities) {
            update_entity(e, input);
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
        level->pixels.physics().SetDebugDraw(&debug_renderer);
        level->pixels.physics().DebugDraw();
        shape_renderer.end_frame();

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