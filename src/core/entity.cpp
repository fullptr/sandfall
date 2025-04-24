#include "entity.hpp"
#include "world.hpp"

#include <box2d/b2_body.h>

namespace sand {
namespace {

static constexpr auto player_id = 1;
static constexpr auto enemy_id = 2;

auto update_player(entity& e, const input& in, double dt) -> void
{
    const bool on_ground = !e.floors.empty();
    const bool can_move_left = e.num_left_contacts == 0;
    const bool can_move_right = e.num_right_contacts == 0;

    const auto vel = e.body->GetLinearVelocity();
    
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

    e.body_fixture->SetFriction(desired_vel != 0 ? 0.2f : 0.95f);

    float vel_change = desired_vel - vel.x;
    float impulse = e.body->GetMass() * vel_change;
    e.body->ApplyLinearImpulseToCenter(b2Vec2(impulse, 0), true);

    if (on_ground) {
        e.double_jump = true;
    }
    if (in.is_down_this_frame(keyboard::W) || in.is_down_this_frame(mouse::left)) {
        if (on_ground || e.double_jump) {
            if (!on_ground) {
                e.double_jump = false;
            }
            float impulse = e.body->GetMass() * 7;
            e.body->ApplyLinearImpulseToCenter(b2Vec2(0, -impulse), true);
        }
    }
}

auto update_enemy(entity& e, const input& in, double dt) -> void
{
    for (const auto curr : e.nearby_entities) {
        const auto user_data = curr->GetUserData();
        if (user_data.pointer == player_id) {
            const auto pos = physics_to_pixel(curr->GetPosition());
            const auto self_pos = entity_centre(e);
            const auto dir = glm::normalize(pos - self_pos);
            e.body->ApplyLinearImpulseToCenter(pixel_to_physics(0.25f * dir), true);
        }
    }
}

}

void contact_listener::PreSolve(b2Contact* contact, const b2Manifold* impulse) 
{
    if (contact->GetFixtureA()->GetUserData().pointer == player_id || contact->GetFixtureB()->GetUserData().pointer == player_id) {
        contact->ResetFriction();
    }
}

void contact_listener::BeginContact(b2Contact* contact)
{
    const auto func = [&](entity& e) {
        const auto a = contact->GetFixtureA();
        const auto b = contact->GetFixtureB();
        if (e.foot_sensor) {
            if (a == e.foot_sensor && !b->IsSensor()) {
                e.floors.insert(b);
            }
            if (b == e.foot_sensor && !a->IsSensor()) {
                e.floors.insert(a);
            }
        }
        if (e.left_sensor) {
            if ((b == e.left_sensor && !a->IsSensor()) || (a == e.left_sensor && !b->IsSensor())) {
                ++e.num_left_contacts;
            }
        }
        if (e.right_sensor) {
            if ((b == e.right_sensor && !a->IsSensor()) || (a == e.right_sensor && !b->IsSensor())) {
                ++e.num_right_contacts;
            }
        }
        if (e.proximity_sensor) {
            if (a == e.proximity_sensor) {
                e.nearby_entities.insert(b->GetBody());
            }
            if (b == e.proximity_sensor) {
                e.nearby_entities.insert(a->GetBody());
            }
        }
    };
    
    func(d_level->player);
    for (auto & e : d_level->entities) {
        func(e);
    }
}

void contact_listener::EndContact(b2Contact* contact) {
    const auto func = [&](entity& e) {
        const auto a = contact->GetFixtureA();
        const auto b = contact->GetFixtureB();
        if (e.foot_sensor) {
            if (a == e.foot_sensor && !b->IsSensor()) {
                e.floors.erase(b);
            }
            if (b == e.foot_sensor && !a->IsSensor()) {
                e.floors.erase(a);
            }
        }
        if (e.left_sensor) {
            if ((b == e.left_sensor && !a->IsSensor()) || (a == e.left_sensor && !b->IsSensor())) {
                --e.num_left_contacts;
            }
        }
        if (e.right_sensor) {
            if ((b == e.right_sensor && !a->IsSensor()) || (a == e.right_sensor && !b->IsSensor())) {
                --e.num_right_contacts;
            }
        }
        if (e.proximity_sensor) {
            if (a == e.proximity_sensor) {
                e.nearby_entities.erase(b->GetBody());
            }
            if (b == e.proximity_sensor) {
                e.nearby_entities.erase(a->GetBody());
            }
        }
    };

    func(d_level->player);
    for (auto & e : d_level->entities) {
        func(e);
    }
}

auto make_player(b2World& world, pixel_pos position) -> entity
{
    entity e;
    e.type = entity_type::player;
    e.spawn_point = position;

    // Create player body
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 1.0f;
    const auto pos = pixel_to_physics(position);
    bodyDef.position.Set(pos.x, pos.y);
    e.body = world.CreateBody(&bodyDef);
    b2MassData md;
    md.mass = 80;
    e.body->SetMassData(&md);
    e.body->GetUserData().pointer = player_id;

    // Set up main body fixture
    {
        const auto half_extents = pixel_to_physics(pixel_pos{5, 10});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 1.0f;
        e.body_fixture = e.body->CreateFixture(&fixtureDef);
    }
    
    // Set up foot sensor
    {
        const auto half_extents = pixel_to_physics(pixel_pos{2, 4});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{0, 10}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        e.foot_sensor = e.body->CreateFixture(&fixtureDef);
    }

     // Set up left sensor
     {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{-5, 0}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        e.left_sensor = e.body->CreateFixture(&fixtureDef);
    }

    // Set up right sensor
    {
        const auto half_extents = pixel_to_physics(pixel_pos{1, 9});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(pixel_pos{5, 0}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        e.right_sensor = e.body->CreateFixture(&fixtureDef);
    }

    return e;
}

auto make_enemy(b2World& world, pixel_pos position) -> entity
{
    entity e;
    e.type = entity_type::enemy;
    e.spawn_point = position;

    // Create player body
    b2BodyDef bodyDef;
    bodyDef.allowSleep = false;
    bodyDef.gravityScale = 0.0f;
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;
    bodyDef.linearDamping = 1.0f;
    const auto pos = pixel_to_physics(position);
    bodyDef.position.Set(pos.x, pos.y);
    e.body = world.CreateBody(&bodyDef);
    b2MassData md;
    md.mass = 10;
    e.body->SetMassData(&md);
    e.body->GetUserData().pointer = enemy_id;

    // Set up main body fixture
    {
        b2CircleShape circleShape;
        circleShape.m_radius = pixel_to_physics(4.0f);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.5f;
        e.body_fixture = e.body->CreateFixture(&fixtureDef);
    }

    // Set up proximity sensor
    {
        b2CircleShape circleShape;
        circleShape.m_radius = pixel_to_physics(100.0f);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.isSensor = true;
        e.proximity_sensor = e.body->CreateFixture(&fixtureDef);
    }

    return e;
}

auto update_entity(entity& e, const input& in, double dt) -> void
{
    switch (e.type) {
        case entity_type::player: {
            update_player(e, in, dt);
        } break;
        case entity_type::enemy: {
            update_enemy(e, in, dt);
        } break;
    }
}

auto respawn_entity(entity& e) -> void
{
    e.body->SetTransform(pixel_to_physics(e.spawn_point), 0);
    e.body->SetLinearVelocity({0, 0});
    e.body->SetAwake(true);
}

auto entity_centre(const entity& e) -> glm::vec2
{
    return physics_to_pixel(e.body->GetPosition());
}

}