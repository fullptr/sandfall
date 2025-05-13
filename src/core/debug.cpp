#include "debug.hpp"

namespace sand {

void draw_circle(b2Vec2 centre, float radius, b2HexColor colour, void* context)
{
    auto& renderer = *static_cast<shape_renderer*>(context);
    const auto c = from_hex(colour);
    const auto r = physics_to_pixel(radius);

    renderer.draw_annulus(physics_to_pixel(centre), c, r - 1.0f, r);
}

void draw_point(b2Vec2 p, float size, b2HexColor colour, void* context)
{
    auto& renderer = *static_cast<shape_renderer*>(context);
    renderer.draw_circle(physics_to_pixel(p), from_hex(colour), 2.0f);
}

void draw_polygon(const b2Vec2* vertices, int vertexCount, b2HexColor colour, void* context)
{
    assert(vertexCount > 1);
    auto& renderer = *static_cast<shape_renderer*>(context);
    const auto c = from_hex(colour);

    for (std::size_t i = 0; i < vertexCount - 1; ++i) {
        const auto p1 = physics_to_pixel(vertices[i]);
        const auto p2 = physics_to_pixel(vertices[i + 1]);
        renderer.draw_line(p1, p2, c, 1);
    }
    const auto p1 = physics_to_pixel(vertices[vertexCount - 1]);
    const auto p2 = physics_to_pixel(vertices[0]);
    renderer.draw_line(p1, p2, c, 1);
}

void draw_segment(b2Vec2 p1, b2Vec2 p2, b2HexColor colour, void* context)
{
    auto& renderer = *static_cast<shape_renderer*>(context);
    renderer.draw_line(physics_to_pixel(p1), physics_to_pixel(p2), from_hex(colour), 2.0f);
}

void draw_solid_capsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor colour, void* context)
{

}

void draw_solid_circle(b2Transform transform, float radius, b2HexColor colour, void* context)
{
    draw_circle(transform.p, radius, colour, context);
}

void draw_solid_polygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor colour, void* context)
{
    draw_polygon(vertices, vertexCount, colour, context);
}

void draw_string(b2Vec2 p, const char* s, b2HexColor colour, void* context)
{

}

void draw_transform(b2Transform transform, void* context)
{

}

}