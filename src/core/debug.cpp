#include "debug.hpp"

namespace sand {

physics_debug_draw::physics_debug_draw(shape_renderer* s) : d_renderer{s} {
    SetFlags(b2Draw::e_shapeBit);
}

void physics_debug_draw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
    assert(vertexCount > 1);
    for (std::size_t i = 0; i < vertexCount - 1; ++i) {
        const auto p1 = physics_to_pixel(vertices[i]);
        const auto p2 = physics_to_pixel(vertices[i + 1]);
        d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
    }
    const auto p1 = physics_to_pixel(vertices[vertexCount - 1]);
    const auto p2 = physics_to_pixel(vertices[0]);
    d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
}

void physics_debug_draw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
    DrawPolygon(vertices, vertexCount, color);
}

void physics_debug_draw::DrawCircle(const b2Vec2& center, float radius, const b2Color& color)
{
    const auto r = physics_to_pixel(radius);
    d_renderer->draw_annulus(
        physics_to_pixel(center),
        {color.r, color.g, color.b, 1.0},
        r - 1.0f,
        r
    );
}

void physics_debug_draw::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color)
{
    DrawCircle(center, radius, color);
}

void physics_debug_draw::DrawSegment(const b2Vec2& bp1, const b2Vec2& bp2, const b2Color& color)
{
    const auto p1 = physics_to_pixel(bp1);
    const auto p2 = physics_to_pixel(bp2);
    d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
}

void physics_debug_draw::DrawTransform(const b2Transform& xf)
{
}

void physics_debug_draw::DrawPoint(const b2Vec2& p, float size, const b2Color& color)
{
    d_renderer->draw_circle(
        physics_to_pixel(p),
        {color.r, color.g, color.b, 1.0},
        2.0f
    );
}

}