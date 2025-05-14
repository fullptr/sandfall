#include "entity.hpp"
#include "world.hpp"
#include "explosion.hpp"

#include <box2d/box2d.h>

namespace sand {

auto add_player(registry& entities, b2WorldId world, pixel_pos position) -> entity
{
    const auto e = entities.create();
    auto& body_comp = entities.emplace<body_component>(e);
    auto& player_comp = entities.emplace<player_component>(e);
    auto& life_comp = entities.emplace<life_component>(e);
    life_comp.spawn_point = position;

    // Create player body
    b2BodyDef def = b2DefaultBodyDef();
    def.type = b2_dynamicBody;
    def.fixedRotation = true;
    def.linearDamping = 1.0f;
    def.position = pixel_to_physics(position);

    body_comp.body = b2CreateBody(world, &def);
    b2Body_SetMassData(body_comp.body, b2MassData{.mass = 80});
    b2Body_SetUserData(body_comp.body, to_user_data(e));

    // Set up main body fixture
    {
        const auto half_extents = pixel_to_physics(pixel_pos{5, 10});
        b2Polygon box = b2MakeBox(half_extents.x, half_extents.y);

        b2ShapeDef def = b2DefaultShapeDef();
        def.density = 1.0f;
        def.material.friction = 1.0f;
        body_comp.body_fixture = b2CreatePolygonShape(body_comp.body, &def, &box);
    }
    
    // Set up foot sensor
    {
        const auto half_extents = pixel_to_physics(pixel_pos{2, 4});
        b2Polygon box = b2MakeOffsetBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{0, 10}), b2MakeRot(0));
        
        b2ShapeDef def = b2DefaultShapeDef();
        def.isSensor = true;
        def.enableSensorEvents = true;
        player_comp.foot_sensor = b2CreatePolygonShape(body_comp.body, &def, &box);
    }

     // Set up left sensor
     {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2Polygon box = b2MakeOffsetBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{-5, 0}), b2MakeRot(0));
        
        b2ShapeDef def = b2DefaultShapeDef();
        def.isSensor = true;
        def.enableSensorEvents = true;
        player_comp.left_sensor = b2CreatePolygonShape(body_comp.body, &def, &box);
    }

    // Set up right sensor
    {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2Polygon box = b2MakeOffsetBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{5, 0}), b2MakeRot(0));
        
        b2ShapeDef def = b2DefaultShapeDef();
        def.isSensor = true;
        def.enableSensorEvents = true;
        player_comp.right_sensor = b2CreatePolygonShape(body_comp.body, &def, &box);
    }

    return e;
}

auto add_enemy(registry& entities, b2WorldId world, pixel_pos position) -> entity
{
    const auto e = entities.create();
    auto& body_comp = entities.emplace<body_component>(e);
    auto& enemy_comp = entities.emplace<enemy_component>(e);
    auto& life_comp = entities.emplace<life_component>(e);

    life_comp.spawn_point = position;

    // Create player body
    b2BodyDef def = b2DefaultBodyDef();
    def.type = b2_dynamicBody;
    def.enableSleep = false;
    def.gravityScale = 0.0f;
    def.fixedRotation = true;
    def.linearDamping = 1.0f;
    def.position = pixel_to_physics(position);

    body_comp.body = b2CreateBody(world, &def);
    b2Body_SetUserData(body_comp.body, to_user_data(e));
    b2Body_SetMassData(body_comp.body, b2MassData{.mass = 10});

    // Set up main body fixture
    {
        b2Circle circle = {};
        circle.radius = pixel_to_physics(4.0f);

        b2ShapeDef def2 = b2DefaultShapeDef();
        def2.density = 1.0f;
        def2.material.friction = 0.5f;
        body_comp.body_fixture = b2CreateCircleShape(body_comp.body, &def2, &circle);
    }

    // Set up proximity sensor
    {
        b2Circle circle = {};
        circle.radius = pixel_to_physics(100.0f);
        
        b2ShapeDef def = b2DefaultShapeDef();
        def.isSensor = true;
        def.enableSensorEvents = true;
        enemy_comp.proximity_sensor = b2CreateCircleShape(body_comp.body, &def, &circle);
    }

    return e;
}

auto ecs_entity_respawn(const registry& entities, entity e) -> void
{
    assert(entities.has<body_component>(e));
    assert(entities.has<life_component>(e));
    
    auto& body_comp = entities.get<body_component>(e);
    auto& life_comp = entities.get<life_component>(e);

    b2Body_SetTransform(body_comp.body, pixel_to_physics(life_comp.spawn_point), b2Rot(0));
    b2Body_SetLinearVelocity(body_comp.body, {0, 0});
    b2Body_SetAwake(body_comp.body, true);
}

auto ecs_entity_centre(const registry& entities, entity e) -> glm::vec2
{
    assert(entities.has<body_component>(e));
    assert(b2Body_IsValid(entities.get<body_component>(e).body));
    return physics_to_pixel(b2Body_GetPosition(entities.get<body_component>(e).body));
}

auto to_user_data(entity e) -> void*
{
    return std::bit_cast<void*>(e);
}

auto from_user_data(void* data) -> entity
{
    return std::bit_cast<entity>(data);
}


}