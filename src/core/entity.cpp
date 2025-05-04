#include "entity.hpp"
#include "world.hpp"

#include <box2d/b2_body.h>

namespace sand {
namespace {

static constexpr auto player_id = 1;
static constexpr auto enemy_id = 2;

static_assert(sizeof(std::uintptr_t) == sizeof(entity));

auto update_player(registry& entities, entity e, const input& in) -> void
{
    auto [body_comp, player_comp] = entities.get_all<body_component, player_component>(e);
    
    const bool on_ground = !player_comp.floors.empty();
    const bool can_move_left = player_comp.num_left_contacts == 0;
    const bool can_move_right = player_comp.num_right_contacts == 0;

    const auto vel = body_comp.body->GetLinearVelocity();
    
    auto direction = 0;
    if (can_move_left && in.is_down(keyboard::A)) {
        direction -= 1;
    }
    if (can_move_right && in.is_down(keyboard::D)) {
        direction += 1;
    }
    
    const auto max_vel = 5.0f;
    auto desired_vel = 0.0f;
    if (direction == -1) { // left
        if (vel.x > -max_vel) desired_vel = b2Max(vel.x - max_vel, -max_vel);
    } else if (direction == 1) { // right
        if (vel.x < max_vel) desired_vel = b2Min(vel.x + max_vel, max_vel);
    }

    body_comp.body_fixture->SetFriction(desired_vel != 0 ? 0.2f : 0.95f);

    float vel_change = desired_vel - vel.x;
    float impulse = body_comp.body->GetMass() * vel_change;
    body_comp.body->ApplyLinearImpulseToCenter(b2Vec2(impulse, 0), true);

    if (on_ground) {
        player_comp.double_jump = true;
        player_comp.ground_pound = true;
    }
}

auto update_enemy(registry& entities, entity e) -> void
{
    if (entities.has<enemy_component>(e)) {
        auto [body_comp, enemy_comp] = entities.get_all<body_component, enemy_component>(e);
        for (const auto curr : enemy_comp.nearby_entities) {
            if (entities.has<player_component>(curr)) {
                auto& curr_body_comp = entities.get<body_component>(curr);
                const auto pos = physics_to_pixel(curr_body_comp.body->GetPosition());
                const auto self_pos = ecs_entity_centre(entities, e);
                const auto dir = glm::normalize(pos - self_pos);
                body_comp.body->ApplyLinearImpulseToCenter(pixel_to_physics(0.25f * dir), true);
            }
        }
    }
}

auto player_handle_event(registry& entities, entity e, const event& ev) -> void
{
    auto [body_comp, player_comp] = entities.get_all<body_component, player_component>(e);

    const bool on_ground = !player_comp.floors.empty();
    if (const auto inner = ev.get_if<keyboard_pressed_event>()) {
        if (inner->key == keyboard::space) {
            if (on_ground || player_comp.double_jump) {
                if (!on_ground) {
                    player_comp.double_jump = false;
                }
                const auto impulse = body_comp.body->GetMass() * 7;
                body_comp.body->ApplyLinearImpulseToCenter(b2Vec2(0, -impulse), true);
            }
        }
        else if (inner->key == keyboard::S) {
            if (player_comp.ground_pound) {
                player_comp.ground_pound = false;
                const auto impulse = body_comp.body->GetMass() * 7;
                body_comp.body->ApplyLinearImpulseToCenter(b2Vec2(0, impulse), true);
            }
        }
    }
    else if (const auto inner = ev.get_if<mouse_pressed_event>()) {
        if (inner->button == mouse::left) {
            std::print("spawning bullet\n");
        }
    }
}

}

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

auto ecs_on_update(registry& entities, const input& in) -> void
{
    for (auto e : entities.view<player_component>()) {
        update_player(entities, e, in);
    }
    for (auto e : entities.view<enemy_component>()) {
        update_enemy(entities, e);
    }
}

auto ecs_on_event(registry& entities, const event& ev) -> void
{
    for (auto e : entities.view<body_component, player_component>()) {
        player_handle_event(entities, e, ev);
    }
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