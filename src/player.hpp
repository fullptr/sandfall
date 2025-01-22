#pragma once
#include <print>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "utility.hpp"
#include "mouse.hpp"

namespace sand {

class player_controller {
    int      d_width;
    int      d_height;
    b2World* d_world;
    b2Body*  d_body        = nullptr;
    bool     d_double_jump = false;

public:
    // width and height are in pixel space
    player_controller(b2World& world, int width, int height)
        : d_width{width}
        , d_height{width}
        , d_world{&world}
    {
        // Create player body
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        const auto position = pixel_to_physics({200.0f, 100.0f});
        const auto dimensions = pixel_to_physics({width, width});
        bodyDef.position.Set(position.x, position.y);
        d_body = world.CreateBody(&bodyDef);
        d_body->SetFixedRotation(true);
        d_body->SetLinearDamping(0.9f);

        b2PolygonShape dynamicBox;
        dynamicBox.SetAsBox(dimensions.x / 2, dimensions.y / 2);

        b2CircleShape circle;
        circle.m_radius = dimensions.x / 2.0;

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circle;
        fixtureDef.density = 1;
        d_body->CreateFixture(&fixtureDef);
    }

    void update(const sand::keyboard& k) {

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

        if (can_move_left && k.is_down(sand::keyboard_key::A)) {
            const auto v = d_body->GetLinearVelocity();
            d_body->SetLinearVelocity({-3.0f, v.y});
        }

        if (can_move_right && k.is_down(sand::keyboard_key::D)) {
            const auto v = d_body->GetLinearVelocity();
            d_body->SetLinearVelocity({3.0f, v.y});
        }

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

    auto get_body() const -> const b2Body* {
        return d_body;
    }

    auto get_contacts() const -> const b2ContactEdge* {
        return d_body->GetContactList();
    }

    auto pos_physics() const -> const b2Vec2& {
        return d_body->GetPosition();
    }

    auto pos_pixel() const {
        return physics_to_pixel(pos_physics());
    }

    auto width_physics() const -> int {
        return pixel_to_physics((float)d_width);
    }

    auto width_pixel() const -> int {
        return d_width;
    }

    auto height_physics() const -> int {
        return pixel_to_physics((float)d_height);
    }

    auto rect_pixels() const -> glm::vec4 {
        auto pos = physics_to_pixel(d_body->GetPosition());
        return glm::vec4{pos.x, pos.y, d_width, d_height};
    }

    auto angle() const -> float {
        return d_body->GetAngle();
    }
};

}