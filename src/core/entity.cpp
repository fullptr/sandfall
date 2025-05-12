#include "entity.hpp"
#include "world.hpp"
#include "explosion.hpp"

#include <box2d/box2d.h>

namespace sand {

//void contact_listener::PreSolve(b2Contact* contact, const b2Manifold*) 
//{
//    const auto a = static_cast<entity>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
//    const auto b = static_cast<entity>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);
//
//    const auto a_is_player = d_level->entities.valid(a) && d_level->entities.has<player_component>(a);
//    const auto b_is_player = d_level->entities.valid(b) && d_level->entities.has<player_component>(b);
//
//    if (a_is_player || b_is_player) {
//        contact->ResetFriction();
//    }
//}
//
//void contact_listener::begin_contact(b2Fixture* curr, b2Fixture* other)
//{
//    const auto curr_entity = static_cast<entity>(curr->GetBody()->GetUserData().pointer);
//    const auto other_entity = static_cast<entity>(other->GetBody()->GetUserData().pointer);
//    
//    if (!d_level->entities.valid(curr_entity)) return;
//
//    if (d_level->entities.has<grenade_component>(curr_entity) 
//        && !other->IsSensor()
//        && (!d_level->entities.valid(other_entity) || !d_level->entities.has<player_component>(other_entity)))
//    {
//        d_level->entities.mark_for_death(curr_entity);
//        const auto pos = ecs_entity_centre(d_level->entities, curr_entity);
//        apply_explosion(d_level->pixels, pixel_pos::from_ivec2(pos), explosion{.min_radius=5, .max_radius=10, .scorch=15});   
//    }
//    
//    if (d_level->entities.has<player_component>(curr_entity)
//        && !other->IsSensor())
//    {
//        auto& comp = d_level->entities.get<player_component>(curr_entity);
//        if (comp.foot_sensor && curr == comp.foot_sensor) {
//            comp.floors.insert(other);
//        }
//        if (comp.left_sensor && curr == comp.left_sensor) {
//            ++comp.num_left_contacts;
//        }
//        if (comp.right_sensor && curr == comp.right_sensor) {
//            ++comp.num_right_contacts;
//        }
//    }
//    
//    if (d_level->entities.has<enemy_component>(curr_entity) 
//        && d_level->entities.valid(other_entity))
//    {
//        auto& comp = d_level->entities.get<enemy_component>(curr_entity);
//        if (comp.proximity_sensor && curr == comp.proximity_sensor && d_level->entities.valid(other_entity)) {
//            comp.nearby_entities.insert(other_entity);
//        }
//    }
//}
//
//void contact_listener::end_contact(b2Fixture* curr, b2Fixture* other)
//{
//    const auto curr_entity = static_cast<entity>(curr->GetBody()->GetUserData().pointer);
//    const auto other_entity = static_cast<entity>(other->GetBody()->GetUserData().pointer);
//    
//    if (!d_level->entities.valid(curr_entity)) return;
//
//    if (d_level->entities.has<player_component>(curr_entity)
//        && !other->IsSensor())
//    {
//        auto& comp = d_level->entities.get<player_component>(curr_entity);
//        if (comp.foot_sensor && curr == comp.foot_sensor) {
//            comp.floors.erase(other);
//        }
//        if (comp.left_sensor && curr == comp.left_sensor) {
//            --comp.num_left_contacts;
//        }
//        if (comp.right_sensor && curr == comp.right_sensor) {
//            --comp.num_right_contacts;
//        }
//    }
//    
//    if (d_level->entities.has<enemy_component>(curr_entity) 
//        && d_level->entities.valid(other_entity))
//    {
//        auto& comp = d_level->entities.get<enemy_component>(curr_entity);
//        if (comp.proximity_sensor && curr == comp.proximity_sensor && d_level->entities.valid(other_entity)) {
//            comp.nearby_entities.erase(other_entity);
//        }
//    }
//}
//
//void contact_listener::BeginContact(b2Contact* contact)
//{
//    const auto a = contact->GetFixtureA();
//    const auto b = contact->GetFixtureB();
//
//    begin_contact(a, b);
//    begin_contact(b, a);
//}
//
//void contact_listener::EndContact(b2Contact* contact)
//{
//    const auto a = contact->GetFixtureA();
//    const auto b = contact->GetFixtureB();
//
//    end_contact(a, b);
//    end_contact(b, a);
//}

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
    b2Body_SetUserData(body_comp.body, (void*)(std::uintptr_t)e); // TODO: Make this safer!!

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
        b2Polygon box = b2MakeBox(half_extents.x, half_extents.y);
        b2Transform t = {};
        t.p = pixel_to_physics(pixel_pos{0, 10});
        b2TransformPolygon(t, &box);
        
        b2ShapeDef def = b2DefaultShapeDef();
        def.isSensor = true;
        player_comp.foot_sensor = b2CreatePolygonShape(body_comp.body, &def, &box);
    }

     // Set up left sensor
     {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2Polygon box = b2MakeBox(half_extents.x, half_extents.y);
        b2Transform t = {};
        t.p = pixel_to_physics(pixel_pos{-5, 0});
        b2TransformPolygon(t, &box);
        
        b2ShapeDef def = b2DefaultShapeDef();
        def.isSensor = true;
        player_comp.left_sensor = b2CreatePolygonShape(body_comp.body, &def, &box);
    }

    // Set up right sensor
    {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2Polygon box = b2MakeBox(half_extents.x, half_extents.y);
        b2Transform t = {};
        t.p = pixel_to_physics(pixel_pos{5, 0});
        b2TransformPolygon(t, &box);
        
        b2ShapeDef def = b2DefaultShapeDef();
        def.isSensor = true;
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
    b2Body_SetUserData(body_comp.body, (void*)(std::uintptr_t)e);
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

}