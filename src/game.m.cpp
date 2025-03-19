#include "world.hpp"
#include "pixel.hpp"
#include "config.hpp"
#include "utility.hpp"
#include "editor.hpp"
#include "camera.hpp"
#include "update_rigid_bodies.hpp"
#include "explosion.hpp"
#include "mouse.hpp"
#include "player.hpp"
#include "world_save.hpp"
#include "renderer.hpp"
#include "shape_renderer.hpp"
#include "window.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <box2d/box2d.h>
#include <cereal/archives/binary.hpp>

#include <memory>
#include <print>
#include <fstream>
#include <cmath>
#include <span>

auto save_level(const std::string& file_path, const sand::level& w) -> void
{
    auto file = std::ofstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryOutputArchive{file};

    auto save = sand::world_save{
        .pixels = w.pixels.pixels(),
        .width = w.pixels.width(),
        .height = w.pixels.height(),
        .spawn_point = w.spawn_point
    };

    archive(save);
}

auto load_level(const std::string& file_path) -> std::unique_ptr<sand::level>
{
    auto file = std::ifstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryInputArchive{file};

    auto save = sand::world_save{};
    archive(save);

    auto w = std::make_unique<sand::level>(save.width, save.height, save.pixels);
    w->spawn_point = save.spawn_point;
    w->player.set_position(save.spawn_point);
    return w;
}

auto new_level(int chunks_width, int chunks_height) -> std::unique_ptr<sand::level>
{
    const auto width = sand::config::chunk_size * chunks_width;
    const auto height = sand::config::chunk_size * chunks_height;
    return std::make_unique<sand::level>(
        width,
        height,
        std::vector<sand::pixel>(width * height, sand::pixel::air())
    );
}

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
        d_renderer->draw_annulus(
            sand::physics_to_pixel(center),
            {color.r, color.g, color.b, 1.0},
            0.8f * sand::physics_to_pixel(radius),
            sand::physics_to_pixel(radius)
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
    auto window          = sand::window{"sandfall", 1280, 720};
    auto editor          = sand::editor{};
    auto mouse           = sand::mouse{};
    auto keyboard        = sand::keyboard{};
    auto level           = new_level(4, 4);
    auto world_renderer  = sand::renderer{level->pixels.width(), level->pixels.height()};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};
    auto debug_draw      = physics_debug_draw{&shape_renderer};

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

        const auto mouse_pos = pixel_at_mouse(mouse, camera);
        switch (editor.brush_type) {
            break; case 0:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto coord = mouse_pos + sand::random_from_circle(editor.brush_size);
                    if (level->pixels.valid(coord)) {
                        level->pixels.set(coord, editor.get_pixel());
                        updated = true;
                    }
                }
            break; case 1:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto half_extent = (int)(editor.brush_size / 2);
                    for (int x = mouse_pos.x - half_extent; x != mouse_pos.x + half_extent + 1; ++x) {
                        for (int y = mouse_pos.y - half_extent; y != mouse_pos.y + half_extent + 1; ++y) {
                            if (level->pixels.valid({x, y})) {
                                level->pixels.set({x, y}, editor.get_pixel());
                                updated = true;
                            }
                        }
                    }
                }
            break; case 2:
                if (mouse.is_down_this_frame(sand::mouse_button::left)) {
                    sand::apply_explosion(level->pixels, mouse_pos, sand::explosion{
                        .min_radius = 40.0f, .max_radius = 45.0f, .scorch = 10.0f
                    });
                    updated = true;
                }
        }

        const auto mouse_actual = mouse_pos_world_space(mouse, camera);
        const auto mouse_pixel = pixel_at_mouse(mouse, camera);

        // Render and display the world
        world_renderer.bind();
        if (updated) {
            world_renderer.update(*level, editor.show_chunks, camera);
        }
        world_renderer.draw();

        shape_renderer.begin_frame(camera);

        // TODO: Replace with actual sprite data
        shape_renderer.draw_circle(level->player.centre(), {1.0, 1.0, 0.0, 1.0}, 3);

        if (editor.show_physics) {
            level->pixels.physics().SetDebugDraw(&debug_draw);
            level->pixels.physics().DebugDraw();
        }

        if (editor.show_spawn) {
            shape_renderer.draw_circle(level->spawn_point, {0, 1, 0, 1}, 1.0);
        }

        shape_renderer.end_frame();
        

        window.end_frame();
    }
    
    return 0;
}