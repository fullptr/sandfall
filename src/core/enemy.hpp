#pragma once
#include <print>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "utility.hpp"
#include "mouse.hpp"

namespace sand {

class enemy_controller {
    b2World*   d_world;
    b2Body*    d_body         = nullptr;
    b2Fixture* d_fixture      = nullptr;
    b2Fixture* d_footSensor   = nullptr;
    b2Fixture* d_left_sensor  = nullptr;
    b2Fixture* d_right_sensor = nullptr;

    bool d_double_jump        = false;
    int  d_num_left_contacts  = 0;
    int  d_num_right_contacts = 0;

    std::unordered_set<b2Fixture*> d_floors;

public:
    auto floors() const -> const std::unordered_set<b2Fixture*>& { return d_floors; }
    
    // width and height are in pixel space
    enemy_controller(b2World& world)
        : d_world{&world}
    {
        // Create player body
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.fixedRotation = true;
        bodyDef.linearDamping = 1.0f;
        const auto position = pixel_to_physics({200, 100});
        bodyDef.position.Set(position.x, position.y);
        d_body = world.CreateBody(&bodyDef);
        b2MassData md;
        md.mass = 80;
        d_body->SetMassData(&md);

        // Set up main body fixture
        {
            const auto half_extents = pixel_to_physics({5, 10});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y);
    
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.density = 1.0f;
            fixtureDef.friction = 1.0f;
            d_fixture = d_body->CreateFixture(&fixtureDef);
        }
        
        // Set up foot sensor
        {
            const auto half_extents = pixel_to_physics({2, 4});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics({0, 10}), 0);
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.isSensor = true;
            d_footSensor = d_body->CreateFixture(&fixtureDef);
        }

         // Set up left sensor
         {
            const auto half_extents = pixel_to_physics({1, 9});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics({-5, 0}), 0);
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.isSensor = true;
            d_left_sensor = d_body->CreateFixture(&fixtureDef);
        }

        // Set up right sensor
        {
            const auto half_extents = pixel_to_physics({1, 9});
            b2PolygonShape shape;
            shape.SetAsBox(half_extents.x, half_extents.y, pixel_to_physics({5, 0}), 0);
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.isSensor = true;
            d_right_sensor = d_body->CreateFixture(&fixtureDef);
        }
    }

    auto set_position(pixel_pos pos) -> void
    {
        d_body->SetTransform(pixel_to_physics(pos), 0);
        d_body->SetLinearVelocity({0, 0});
        d_body->SetAwake(true);
    }

    void update(const sand::keyboard& k)
    {
        const bool on_ground = !d_floors.empty();
        const bool can_move_left = d_num_left_contacts == 0;
        const bool can_move_right = d_num_right_contacts == 0;

        const auto vel = d_body->GetLinearVelocity();
        
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

        d_fixture->SetFriction(desired_vel != 0 ? 0.2f : 0.95f);

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