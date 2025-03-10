#pragma once
#include <print>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "utility.hpp"
#include "mouse.hpp"

namespace sand {

class player_controller {
    int      d_radius;
    b2World* d_world;
    b2Body*  d_body        = nullptr;
    bool     d_double_jump = false;

public:
    // width and height are in pixel space
    player_controller(b2World& world, int radius)
        : d_radius{radius}
        , d_world{&world}
    {
        // Create player body
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        const auto position = pixel_to_physics({200.0f, 100.0f});
        bodyDef.position.Set(position.x, position.y);
        d_body = world.CreateBody(&bodyDef);
        d_body->SetFixedRotation(true);
        d_body->SetLinearDamping(0.9f);

        b2CircleShape circle;
        circle.m_radius = pixel_to_physics(d_radius);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circle;
        fixtureDef.density = 1;
        fixtureDef.friction = 1;
        d_body->CreateFixture(&fixtureDef);
    }

    auto set_position(glm::ivec2 pos) -> void
    {
        d_body->SetTransform(pixel_to_physics(pos), 0);
        d_body->SetLinearVelocity({0, 0});
        d_body->SetAwake(true);
    }

    void update(const sand::keyboard& k)
    {
        bool on_ground = false;
        bool can_move_left = true;
        bool can_move_right = true;

        for (auto c = d_body->GetContactList(); c; c = c->next) {
            if (!c->contact->IsTouching()) continue;

            const auto normal = -c->other->GetWorldVector(c->contact->GetManifold()->localNormal);

            const auto up_dot = b2Dot(normal, b2Vec2(0.0, 1.0));
            const auto left_dot = b2Dot(normal, b2Vec2(-1.0, 0.0));
            const auto right_dot = b2Dot(normal, b2Vec2(1.0, 0.0));

            if (up_dot > 0.7) { on_ground = true; }
            if (left_dot > 0.7) { can_move_left = false; }
            if (right_dot > 0.7) { can_move_right = false; }
        }

        float force = 0;
        const auto vel = d_body->GetLinearVelocity();
        float desired_vel = 0;

        if (can_move_left && k.is_down(sand::keyboard_key::A)) {
            if (vel.x > -5) desired_vel -= 5;
        }

        if (can_move_right && k.is_down(sand::keyboard_key::D)) {
            if (vel.x < 5) desired_vel += 5;
        }

        float vel_change = desired_vel - vel.x;
        float impulse = d_body->GetMass() * vel_change; //disregard time factor
        d_body->ApplyLinearImpulse(b2Vec2(impulse, 0), d_body->GetWorldCenter(), true);

        if (on_ground) {
            d_double_jump = true;
        }
        if (k.is_down_this_frame(sand::keyboard_key::W)) {
            if (on_ground || d_double_jump) {
                if (!on_ground) {
                    d_double_jump = false;
                }
                const auto v = d_body->GetLinearVelocity();
                d_body->SetLinearVelocity({v.x, -5.0f});
            }
        }
    }

    auto centre() const {
        return physics_to_pixel(d_body->GetPosition());
    }

    auto radius() const -> int {
        return d_radius;
    }
};

}