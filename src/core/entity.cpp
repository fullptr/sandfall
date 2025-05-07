#include "entity.hpp"
#include "world.hpp"

#include <box2d/b2_body.h>

namespace sand {

void contact_listener::PreSolve(b2Contact* contact, const b2Manifold*) 
{
    const auto a = static_cast<entity>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
    const auto b = static_cast<entity>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

    if (d_level->entities.has<player_component>(a) || d_level->entities.has<player_component>(b)) {
        contact->ResetFriction();
    }
}

void contact_listener::begin_contact(b2Fixture* curr, b2Fixture* other)
{
    const auto curr_entity = static_cast<entity>(curr->GetBody()->GetUserData().pointer);
    const auto other_entity = static_cast<entity>(other->GetBody()->GetUserData().pointer);

    if (d_level->entities.has<grenade_component>(curr_entity) 
        && !d_level->entities.has<player_component>(other_entity)
        && !other->IsSensor())
    {
        std::print("grenade has impacted\n");
    }
    
    if (d_level->entities.has<player_component>(curr_entity) && !other->IsSensor()) {
        auto& comp = d_level->entities.get<player_component>(curr_entity);
        if (comp.foot_sensor && curr == comp.foot_sensor) {
            comp.floors.insert(other);
        }
        if (comp.left_sensor && curr == comp.left_sensor) {
            ++comp.num_left_contacts;
        }
        if (comp.right_sensor && curr == comp.right_sensor) {
            ++comp.num_right_contacts;
        }
    }
    
    if (d_level->entities.has<enemy_component>(curr_entity)) {
        auto& comp = d_level->entities.get<enemy_component>(curr_entity);
        if (comp.proximity_sensor && curr == comp.proximity_sensor && d_level->entities.valid(other_entity)) {
            comp.nearby_entities.insert(other_entity);
        }
    }
}

void contact_listener::end_contact(b2Fixture* curr, b2Fixture* other)
{
    const auto curr_entity = static_cast<entity>(curr->GetBody()->GetUserData().pointer);
    const auto other_entity = static_cast<entity>(other->GetBody()->GetUserData().pointer);
    
    if (d_level->entities.has<player_component>(curr_entity) && !other->IsSensor()) {
        auto& comp = d_level->entities.get<player_component>(curr_entity);
        if (comp.foot_sensor && curr == comp.foot_sensor) {
            comp.floors.erase(other);
        }
        if (comp.left_sensor && curr == comp.left_sensor) {
            --comp.num_left_contacts;
        }
        if (comp.right_sensor && curr == comp.right_sensor) {
            --comp.num_right_contacts;
        }
    }
    
    if (d_level->entities.has<enemy_component>(curr_entity)) {
        auto& comp = d_level->entities.get<enemy_component>(curr_entity);
        if (comp.proximity_sensor && curr == comp.proximity_sensor && d_level->entities.valid(other_entity)) {
            comp.nearby_entities.erase(other_entity);
        }
    }
}

void contact_listener::BeginContact(b2Contact* contact)
{
    const auto a = contact->GetFixtureA();
    const auto b = contact->GetFixtureB();

    begin_contact(a, b);
    begin_contact(b, a);
}

void contact_listener::EndContact(b2Contact* contact)
{
    const auto a = contact->GetFixtureA();
    const auto b = contact->GetFixtureB();

    end_contact(a, b);
    end_contact(b, a);
}

auto add_player(registry& entities, b2World& world, pixel_pos position) -> entity
{
    const auto e = entities.create();
    auto& body_comp = entities.emplace<body_component>(e);
    auto& player_comp = entities.emplace<player_component>(e);
    auto& life_comp = entities.emplace<life_component>(e);
    life_comp.spawn_point = position;

    // Create player body
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 1.0f;
    const auto pos = pixel_to_physics(position);
    bodyDef.position.Set(pos.x, pos.y);
    body_comp.body = world.CreateBody(&bodyDef);
    body_comp.body->GetUserData().pointer = static_cast<std::uintptr_t>(e);
    b2MassData md;
    md.mass = 80;
    body_comp.body->SetMassData(&md);

    // Set up main body fixture
    {
        const auto half_extents = pixel_to_physics(pixel_pos{5, 10});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 1.0f;
        body_comp.body_fixture = body_comp.body->CreateFixture(&fixtureDef);
    }
    
    // Set up foot sensor
    {
        const auto half_extents = pixel_to_physics(pixel_pos{2, 4});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{0, 10}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        player_comp.foot_sensor = body_comp.body->CreateFixture(&fixtureDef);
    }

     // Set up left sensor
     {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{-5, 0}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        player_comp.left_sensor = body_comp.body->CreateFixture(&fixtureDef);
    }

    // Set up right sensor
    {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{5, 0}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        player_comp.right_sensor = body_comp.body->CreateFixture(&fixtureDef);
    }

    return e;
}

auto add_enemy(registry& entities, b2World& world, pixel_pos position) -> entity
{
    const auto e = entities.create();
    auto& body_comp = entities.emplace<body_component>(e);
    auto& enemy_comp = entities.emplace<enemy_component>(e);
    auto& life_comp = entities.emplace<life_component>(e);

    life_comp.spawn_point = position;

    // Create player body
    b2BodyDef bodyDef;
    bodyDef.allowSleep = false;
    bodyDef.gravityScale = 0.0f;
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 1.0f;
    const auto pos = pixel_to_physics(position);
    bodyDef.position.Set(pos.x, pos.y);
    body_comp.body = world.CreateBody(&bodyDef);
    body_comp.body->GetUserData().pointer = static_cast<std::uintptr_t>(e);
    b2MassData md;
    md.mass = 10;
    body_comp.body->SetMassData(&md);

    // Set up main body fixture
    {
        b2CircleShape circleShape;
        circleShape.m_radius = pixel_to_physics(4.0f);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.5f;
        body_comp.body_fixture = body_comp.body->CreateFixture(&fixtureDef);
    }

    // Set up proximity sensor
    {
        b2CircleShape circleShape;
        circleShape.m_radius = pixel_to_physics(100.0f);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.isSensor = true;
        enemy_comp.proximity_sensor = body_comp.body->CreateFixture(&fixtureDef);
    }

    return e;
}

auto ecs_entity_respawn(const registry& entities, entity e) -> void
{
    assert(entities.has<body_component>(e));
    assert(entities.has<life_component>(e));
    
    auto& body_comp = entities.get<body_component>(e);
    auto& life_comp = entities.get<life_component>(e);

    body_comp.body->SetTransform(pixel_to_physics(life_comp.spawn_point), 0);
    body_comp.body->SetLinearVelocity({0, 0});
    body_comp.body->SetAwake(true);
}

auto ecs_entity_centre(const registry& entities, entity e) -> glm::vec2
{
    assert(entities.has<body_component>(e));
    return physics_to_pixel(entities.get<body_component>(e).body->GetPosition());
}

}