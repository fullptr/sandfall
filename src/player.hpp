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
        , d_height{height}
        , d_world{&world}
    {
        // Create player body
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        const auto position = pixel_to_physics({200.0f, 100.0f});
        const auto dimensions = pixel_to_physics({width, height});
        bodyDef.position.Set(position.x, position.y);
        d_body = world.CreateBody(&bodyDef);
        d_body->SetFixedRotation(true);

        {
            b2PolygonShape dynamicBox;
            dynamicBox.SetAsBox(dimensions.x / 2, dimensions.y / 2);

            b2CircleShape circle;
            circle.m_radius = dimensions.x / 2;

            b2FixtureDef fixtureDef;
            fixtureDef.shape = &circle;
            fixtureDef.density = 12.0f;
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

    void handle_input(sand::keyboard& k) {
        // Move left
        if (k.is_down(sand::keyboard_key::A)) {
            const auto v = d_body->GetLinearVelocity();
            d_body->SetLinearVelocity({-3.0f, v.y});
        }

        // Move right
        if (k.is_down(sand::keyboard_key::D)) {
            const auto v = d_body->GetLinearVelocity();
            d_body->SetLinearVelocity({3.0f, v.y});
        }

        auto bottom = pos_physics();
        bottom.y += height_physics() / 2;
        const bool on_ground = is_on_ground(*d_world, bottom);
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