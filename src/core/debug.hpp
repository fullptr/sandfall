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

}