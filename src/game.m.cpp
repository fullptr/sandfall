#include "common.hpp"
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

// TODO: Move to library?
class physics_debug_draw : public b2Draw
{
    sand::shape_renderer* d_renderer;

public:
    physics_debug_draw(sand::shape_renderer* s) : d_renderer{s} {
        SetFlags(b2Draw::e_shapeBit);
    }

    void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
    {
        assert(vertexCount > 1);
        for (std::size_t i = 0; i < vertexCount - 1; ++i) {
            const auto p1 = sand::physics_to_pixel(vertices[i]);
            const auto p2 = sand::physics_to_pixel(vertices[i + 1]);
            d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
        }
        const auto p1 = sand::physics_to_pixel(vertices[vertexCount - 1]);
        const auto p2 = sand::physics_to_pixel(vertices[0]);
        d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
    }
    
    void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
    {
        DrawPolygon(vertices, vertexCount, color);
    }
    
    void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override
    {
        const auto r = sand::physics_to_pixel(radius);
        d_renderer->draw_annulus(
            sand::physics_to_pixel(center),
            {color.r, color.g, color.b, 1.0},
            r - 1.0f,
            r
        );
    }

	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override
    {
        DrawCircle(center, radius, color);
    }

	void DrawSegment(const b2Vec2& bp1, const b2Vec2& bp2, const b2Color& color) override
    {
        const auto p1 = sand::physics_to_pixel(bp1);
        const auto p2 = sand::physics_to_pixel(bp2);
        d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
    }

	void DrawTransform(const b2Transform& xf) override
    {
    }

	void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override
    {
        d_renderer->draw_circle(
            sand::physics_to_pixel(p),
            {color.r, color.g, color.b, 1.0},
            2.0f
        );
    }
};

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
    auto debug_renderer  = physics_debug_draw{&shape_renderer};

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
        level->pixels.physics().SetDebugDraw(&debug_renderer);
        level->pixels.physics().DebugDraw();
        shape_renderer.end_frame();

        
        window.end_frame();
    }
    
    return 0;
}