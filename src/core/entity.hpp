#pragma once
#include <print>
#include <unordered_set>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "apecs.hpp"
#include "utility.hpp"
#include "input.hpp"

namespace sand {

using entity = apx::entity;

class level;
class contact_listener : public b2ContactListener
{
    level* d_level;

    void begin_contact(b2Fixture* curr, b2Fixture* other);
    void end_contact(b2Fixture* curr, b2Fixture* other);

public:
    contact_listener(level* l) : d_level{l} {}

    void PreSolve(b2Contact* contact, const b2Manifold* impulse) override;
    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;
};

struct body_component
{
    b2Body* body = nullptr;
    b2Fixture* body_fixture = nullptr;
};

struct player_component
{
    b2Fixture* foot_sensor  = nullptr;
    b2Fixture* left_sensor  = nullptr;
    b2Fixture* right_sensor = nullptr;
    
    std::unordered_set<b2Fixture*> floors;
    int num_left_contacts  = 0;
    int num_right_contacts = 0;

    bool double_jump = true;
    bool ground_pound = true;
};

struct enemy_component
{
    b2Fixture*                 proximity_sensor = nullptr;
    std::unordered_set<entity> nearby_entities;
};

struct life_component
{
    pixel_pos spawn_point = {0, 0};
    i32       health      = 100;
};

struct grenade_component
{};

using registry = apx::registry<
    body_component,
    player_component,
    enemy_component,
    life_component,
    grenade_component
>;

auto add_player(registry& entities, b2World& world, pixel_pos position) -> entity;
auto add_enemy(registry& entities, b2World& world, pixel_pos position) -> entity;

auto ecs_entity_respawn(const registry& entities, entity e) -> void;
auto ecs_entity_centre(const registry& entities, entity e) -> glm::vec2;

}