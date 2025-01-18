#pragma once
#include <print>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "utility.hpp"
#include "mouse.hpp"

namespace sand {

class ray_cast_callback : public b2RayCastCallback
{
public:
    bool found = false;
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
                        const b2Vec2& normal, float fraction) override {
        found = true;          
        return 0;         
    }
};

inline auto is_on_ground(const b2World& world, const b2Vec2& bottom) -> bool
{
    auto callback = ray_cast_callback{};
    auto start = bottom;
    auto end = start;
    end.y += 0.75f;
    world.RayCast(&callback, start, end);
    return callback.found;
}

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
        d_body->SetLinearDamping(0.5f);

        {
            b2PolygonShape dynamicBox;
            dynamicBox.SetAsBox(dimensions.x / 2, dimensions.y / 2);

            b2CircleShape circle;
            circle.m_radius = dimensions.x / 2.0;

            b2FixtureDef fixtureDef;
            fixtureDef.shape = &circle;
            fixtureDef.density = 12;
            d_body->CreateFixture(&fixtureDef);
        }

        if (false) {
            b2PolygonShape dynamicBox;
            dynamicBox.SetAsBox(dimensions.x / 2 + 0.01f, dimensions.y / 2 - 0.01f);
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &dynamicBox;
            fixtureDef.density = 12.0f;
            fixtureDef.friction = 0.0f;
            d_body->CreateFixture(&fixtureDef);
        }
    }

    void update(sand::keyboard& k) {

        bool on_ground = false;
        bool can_move_left = true;
        bool can_move_right = true;
        auto* contact = d_body->GetContactList();
        for (auto contact = d_body->GetContactList(); contact; contact = contact->next) {
            if (!contact->contact->IsTouching()) continue;
            
            const b2Vec2 normal = contact->contact->GetManifold()->localNormal;
            b2Vec2 worldNormal = contact->other->GetWorldVector(normal);

            const auto dot = b2Dot(worldNormal, b2Vec2(0.0, -1.0));
            if (dot > 0.7) { on_ground = true; }

            const auto leftDot = b2Dot(worldNormal, b2Vec2(1.0, 0.0));
            if (leftDot > 0.7) { can_move_left = false; }

            const auto rightDot = b2Dot(worldNormal, b2Vec2(-1.0, 0.0));
            if (rightDot > 0.7) { can_move_right = false; }
        }

        // Move left
        if (can_move_left && k.is_down(sand::keyboard_key::A)) {
            const auto v = d_body->GetLinearVelocity();
            d_body->SetLinearVelocity({-3.0f, v.y});
        }

        // Move right
        if (can_move_right && k.is_down(sand::keyboard_key::D)) {
            const auto v = d_body->GetLinearVelocity();
            d_body->SetLinearVelocity({3.0f, v.y});
        }

        if (on_ground) { d_double_jump = true; }
        if (k.is_down_this_frame(sand::keyboard_key::W)) {
            if (on_ground || d_double_jump) {
                if (!on_ground) d_double_jump = false;
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

    auto width_physics() const -> int {
        return pixel_to_physics((float)d_width);
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