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

//class level;
//class contact_listener : public b2ContactListener
//{
//    level* d_level;
//
//    void begin_contact(b2Fixture* curr, b2Fixture* other);
//    void end_contact(b2Fixture* curr, b2Fixture* other);
//
//public:
//    contact_listener(level* l) : d_level{l} {}
//
//    void PreSolve(b2Contact* contact, const b2Manifold* impulse) override;
//    void BeginContact(b2Contact* contact) override;
//    void EndContact(b2Contact* contact) override;
//};

struct body_component
{
    b2BodyId body = b2_nullBodyId;
    b2ShapeId body_fixture = b2_nullShapeId;
};

struct player_component
{
    b2ShapeId foot_sensor  = b2_nullShapeId;
    b2ShapeId left_sensor  = b2_nullShapeId;
    b2ShapeId right_sensor = b2_nullShapeId;
    
    std::unordered_set<b2ShapeId> floors;
    int num_left_contacts  = 0;
    int num_right_contacts = 0;

    bool double_jump = true;
    bool ground_pound = true;
};

struct enemy_component
{
    b2ShapeId                  proximity_sensor = b2_nullShapeId;
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

auto add_player(registry& entities, b2WorldId world, pixel_pos position) -> entity;
auto add_enemy(registry& entities, b2WorldId world, pixel_pos position) -> entity;

auto ecs_entity_respawn(const registry& entities, entity e) -> void;
auto ecs_entity_centre(const registry& entities, entity e) -> glm::vec2;

}