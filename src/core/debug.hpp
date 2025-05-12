#pragma once
#include "shape_renderer.hpp"
#include "utility.hpp"

#include <box2d/box2d.h>

namespace sand {

void draw_circle(b2Vec2 centre, float radius, b2HexColor colour, void* context);
void draw_point(b2Vec2 p, float size, b2HexColor colour, void* context);
void draw_polygon(const b2Vec2* vertices, int vertexCount, b2HexColor colour, void* context);
void draw_segment(b2Vec2 p1, b2Vec2 p2, b2HexColor colour, void* context);
void draw_solid_capsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor colour, void* context);
void draw_solid_circle(b2Transform transform, float radius, b2HexColor colour, void* context);
void draw_solid_polygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor colour, void* context);
void draw_string(b2Vec2 p, const char* s, b2HexColor colour, void* context);
void draw_transform(b2Transform transform, void* context);

#if 0
class physics_debug_draw : public b2Draw
{
    shape_renderer* d_renderer;

public:
    physics_debug_draw(shape_renderer* s);
    
    void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
    void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
    void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override;
	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override;
	void DrawSegment(const b2Vec2& bp1, const b2Vec2& bp2, const b2Color& color);
	void DrawTransform(const b2Transform& xf) override;
	void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;
};
#endif

}