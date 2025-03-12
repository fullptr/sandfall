#pragma once
#include <print>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "utility.hpp"
#include "mouse.hpp"

namespace sand {

class player_controller : public b2ContactListener {
    b2World*   d_world;
    b2Body*    d_body         = nullptr;
    b2Fixture* d_fixture      = nullptr;
    b2Fixture* d_footSensor   = nullptr;
    b2Fixture* d_left_sensor  = nullptr;
    b2Fixture* d_right_sensor = nullptr;
    float      d_friction     = 1.0f;

    bool d_double_jump        = false;
    int  d_num_foot_contacts  = 0;
    int  d_num_left_contacts  = 0;
    int  d_num_right_contacts = 0;

public:
    // width and height are in pixel space
    player_controller(b2World& world)
        : d_world{&world}
    {
        // Create player body
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.fixedRotation = true;
        bodyDef.linearDamping = 1.0f;
        const auto position = pixel_to_physics({200.0f, 100.0f});
        bodyDef.position.Set(position.x, position.y);
        d_body = world.CreateBody(&bodyDef);
        b2MassData md;
        md.mass = 80;
        d_body->SetMassData(&md);

        // Set up main body fixture
        {
            const auto half_extents = pixel_to_physics(glm::vec2{5.0f, 10.0f});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y);
    
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.density = 1;
            fixtureDef.friction = d_friction;
            d_fixture = d_body->CreateFixture(&fixtureDef);
        }
        
        // Set up foot sensor
        {
            const auto half_extents = pixel_to_physics(glm::vec2{4.0f, 4.0f});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(glm::vec2{0.0f, 10.0f}), 0);
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.isSensor = true;
            d_footSensor = d_body->CreateFixture(&fixtureDef);
        }

         // Set up left sensor
         {
            const auto half_extents = pixel_to_physics(glm::vec2{1.0f, 9.0f});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(glm::vec2{-5.0f, 0.0f}), 0);
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.isSensor = true;
            d_left_sensor = d_body->CreateFixture(&fixtureDef);
        }

        // Set up right sensor
        {
            const auto half_extents = pixel_to_physics(glm::vec2{1.0f, 9.0f});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics(glm::vec2{5.0f, 0.0f}), 0);
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.isSensor = true;
            d_right_sensor = d_body->CreateFixture(&fixtureDef);
        }

        d_world->SetContactListener(this);
    }

    void PreSolve(b2Contact* contact, const b2ContactImpulse* impulse) {
        if (contact->GetFixtureA() == d_fixture || contact->GetFixtureB() == d_fixture) {
            contact->ResetFriction();
        }
    }

    void BeginContact(b2Contact* contact) {
        if (contact->GetFixtureA() == d_footSensor || contact->GetFixtureB() == d_footSensor) {
            ++d_num_foot_contacts;
        }
        if (contact->GetFixtureA() == d_left_sensor || contact->GetFixtureB() == d_left_sensor) {
            ++d_num_left_contacts;
        }
        if (contact->GetFixtureA() == d_right_sensor || contact->GetFixtureB() == d_right_sensor) {
            ++d_num_right_contacts;
        }
    }

    void EndContact(b2Contact* contact) {
        if (contact->GetFixtureA() == d_footSensor || contact->GetFixtureB() == d_footSensor) {
            --d_num_foot_contacts;
        }
        if (contact->GetFixtureA() == d_left_sensor || contact->GetFixtureB() == d_left_sensor) {
            --d_num_left_contacts;
        }
        if (contact->GetFixtureA() == d_right_sensor || contact->GetFixtureB() == d_right_sensor) {
            --d_num_right_contacts;
        }
    }

    auto set_position(glm::ivec2 pos) -> void
    {
        d_body->SetTransform(pixel_to_physics(pos), 0);
        d_body->SetLinearVelocity({0, 0});
        d_body->SetAwake(true);
    }

    void update(const sand::keyboard& k)
    {
        const bool on_ground = d_num_foot_contacts > 0;
        const bool can_move_left = d_num_left_contacts == 0;
        const bool can_move_right = d_num_right_contacts == 0;

        float force = 0;
        const auto vel = d_body->GetLinearVelocity();
        float desired_vel = 0;

        const auto go_l = can_move_left && k.is_down(sand::keyboard_key::A);
        const auto go_r = can_move_right && k.is_down(sand::keyboard_key::D);
        const auto go_none = !(go_l ^ go_r);
        
        if (go_none) {
            desired_vel = 0.f;
        } else if (go_l) {
            if (vel.x > -5) desired_vel = b2Max(vel.x - 0.5f, -5.0f);
        } else if (go_r) {
            if (vel.x < 5) desired_vel = b2Min(vel.x + 0.5f,  5.0f);;
        }

        if (desired_vel != 0) {
            d_friction = 0.1f;
        } else {
            d_friction = 1.0f;
        }
        d_fixture->SetFriction(d_friction);

        float vel_change = desired_vel - vel.x;
        float impulse = d_body->GetMass() * vel_change;
        d_body->ApplyLinearImpulseToCenter(b2Vec2(impulse, 0), true);

        if (on_ground) {
            d_double_jump = true;
        }
        if (k.is_down_this_frame(sand::keyboard_key::W)) {
            if (on_ground || d_double_jump) {
                if (!on_ground) {
                    d_double_jump = false;
                }
                float impulse = d_body->GetMass() * 7;
                d_body->ApplyLinearImpulseToCenter(b2Vec2(0, -impulse), true);
            }
        }
    }

    auto centre() const {
        return physics_to_pixel(d_body->GetPosition());
    }
};

}