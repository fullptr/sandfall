#pragma once
#include <print>
#include <unordered_set>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "utility.hpp"
#include "mouse.hpp"

namespace sand {

class level;
class contact_listener : public b2ContactListener
{
    level* d_level;

public:
    contact_listener(level* l) : d_level{l} {}

    void PreSolve(b2Contact* contact, const b2Manifold* impulse) override;
    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;
};

enum class entity_type
{
    player,
    enemy,
};

// Possibly will replace with an entity component system in the future,
// but for now just a big bag of data will suffice
struct entity
{
    entity_type type;

    pixel_pos  spawn_point  = {0, 0};
    bool       is_player    = false;

    b2Body*    body         = nullptr;
    b2Fixture* body_fixture = nullptr;
    b2Fixture* foot_sensor  = nullptr;
    b2Fixture* left_sensor  = nullptr;
    b2Fixture* right_sensor = nullptr;

    // Used by the enemy AI to detect the player
    b2Fixture* proximity_sensor = nullptr;

    bool double_jump        = false;
    int  num_left_contacts  = 0;
    int  num_right_contacts = 0;

    std::unordered_set<b2Fixture*> floors;

};

auto make_player(b2World& world, pixel_pos position) -> entity;
auto make_enemy(b2World& world, pixel_pos position) -> entity;
auto update_entity(entity& e, const keyboard& k) -> void;
auto respawn_entity(entity& e) -> void;
auto entity_centre(const entity& e) -> glm::vec2;

}