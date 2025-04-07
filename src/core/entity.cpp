#include "entity.hpp"
#include "world.hpp"

namespace sand {

void contact_listener::PreSolve(b2Contact* contact, const b2Manifold* impulse)  {
    const auto func = [&](entity& e) {
        if (contact->GetFixtureA() == e.fixture || contact->GetFixtureB() == e.fixture) {
            contact->ResetFriction();
        }
    };

    func(d_level->player);
    for (auto & e : d_level->entities) {
        func(e);
    }
}

void contact_listener::BeginContact(b2Contact* contact) {
    const auto func = [&](entity& e) {
        if (contact->GetFixtureA() == e.footSensor) {
            e.floors.insert(contact->GetFixtureB());
        }
        if (contact->GetFixtureB() == e.footSensor) {
            e.floors.insert(contact->GetFixtureA());
        }
        if (contact->GetFixtureA() == e.left_sensor || contact->GetFixtureB() == e.left_sensor) {
            ++e.num_left_contacts;
        }
        if (contact->GetFixtureA() == e.right_sensor || contact->GetFixtureB() == e.right_sensor) {
            ++e.num_right_contacts;
        }
    };
    
    func(d_level->player);
    for (auto & e : d_level->entities) {
        func(e);
    }
}

void contact_listener::EndContact(b2Contact* contact) {
    const auto func = [&](entity& e) {
        if (contact->GetFixtureA() == e.footSensor) {
            e.floors.erase(contact->GetFixtureB());
        }
        if (contact->GetFixtureB() == e.footSensor) {
            e.floors.erase(contact->GetFixtureA());
        }
        if (contact->GetFixtureA() == e.left_sensor || contact->GetFixtureB() == e.left_sensor) {
            --e.num_left_contacts;
        }
        if (contact->GetFixtureA() == e.right_sensor || contact->GetFixtureB() == e.right_sensor) {
            --e.num_right_contacts;
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
    e.spawn_point = position;
    e.is_player = true;

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

    // Set up main body fixture
    {
        const auto half_extents = pixel_to_physics({5, 10});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 1.0f;
        e.fixture = e.body->CreateFixture(&fixtureDef);
    }
    
    // Set up foot sensor
    {
        const auto half_extents = pixel_to_physics({2, 4});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics({0, 10}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        e.footSensor = e.body->CreateFixture(&fixtureDef);
    }

     // Set up left sensor
     {
        const auto half_extents = pixel_to_physics({1, 9});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics({-5, 0}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        e.left_sensor = e.body->CreateFixture(&fixtureDef);
    }

    // Set up right sensor
    {
        const auto half_extents = pixel_to_physics({1, 9});
        b2PolygonShape shape;
        shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics({5, 0}), 0);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.isSensor = true;
        e.right_sensor = e.body->CreateFixture(&fixtureDef);
    }

    return e;
}

auto update_entity(entity& e, const keyboard& k) -> void
{
    const bool on_ground = !e.floors.empty();
    const bool can_move_left = e.num_left_contacts == 0;
    const bool can_move_right = e.num_right_contacts == 0;

    const auto vel = e.body->GetLinearVelocity();
    
    auto direction = 0;
    if (can_move_left && k.is_down(sand::keyboard_key::A)) {
        direction -= 1;
    }
    if (can_move_right && k.is_down(sand::keyboard_key::D)) {
        direction += 1;
    }
    
    const auto max_vel = 5.0f;
    auto desired_vel = 0.0f;
    if (direction == -1) { // left
        if (vel.x > -max_vel) desired_vel = b2Max(vel.x - max_vel, -max_vel);
    } else if (direction == 1) { // right
        if (vel.x < max_vel) desired_vel = b2Min(vel.x + max_vel, max_vel);
    }

    e.fixture->SetFriction(desired_vel != 0 ? 0.2f : 0.95f);

    float vel_change = desired_vel - vel.x;
    float impulse = e.body->GetMass() * vel_change;
    e.body->ApplyLinearImpulseToCenter(b2Vec2(impulse, 0), true);

    if (on_ground) {
        e.double_jump = true;
    }
    if (k.is_down_this_frame(sand::keyboard_key::W)) {
        if (on_ground || e.double_jump) {
            if (!on_ground) {
                e.double_jump = false;
            }
            float impulse = e.body->GetMass() * 7;
            e.body->ApplyLinearImpulseToCenter(b2Vec2(0, -impulse), true);
        }
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