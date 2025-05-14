#pragma once
#include <print>
#include <unordered_set>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "apecs.hpp"
#include "utility.hpp"
#include "input.hpp"

struct shape_id_hash
{
    auto operator()(const b2ShapeId id) const -> std::size_t
    {
        return std::hash<std::uint64_t>{}(std::bit_cast<std::uint64_t>(id));
    }
};

struct shape_id_equal
{
    auto operator()(const b2ShapeId lhs, const b2ShapeId rhs) const -> bool
    {
        return B2_ID_EQUALS(lhs, rhs);
    }
};

namespace sand {

using entity = apx::entity;

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
    
    std::unordered_set<b2ShapeId, shape_id_hash, shape_id_equal> floors;
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

// Used for storing entity values in the user data. The point is not a valid
// pointer! It just encodes the entity.
static_assert(sizeof(entity) == sizeof(void*));
auto to_user_data(entity e) -> void*;
auto from_user_data(void* data) -> entity;

}