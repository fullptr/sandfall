#pragma once
#include <glm/glm.hpp>

#include <bitset>
#include <cstdint>

namespace sand {

enum pixel_flags : std::size_t
{
    is_updated, // 0
    is_falling, // 1
    is_burning, // 2
};

enum class pixel_phase : std::uint8_t
{
    solid,
    liquid,
    gas,
};

enum class pixel_type : std::uint8_t
{
    none,
    sand,
    dirt,
    coal,
    water,
    lava,
    acid,
    rock,
    titanium,
    steam,
    fuse,
    ember,
    oil,
    gunpowder,
    methane,
    battery,
    solder
};

struct pixel_properties
{
    // Movement Controls
    pixel_phase phase               = pixel_phase::solid;
    bool        can_move_diagonally = false;
    float       gravity_factor      = 0.0f;
    float       inertial_resistance = 0.0f;
    int         dispersion_rate     = 0;

    // Simulator Specifics
    bool        always_awake        = false; // If enabled, then the chunk never sleeps

    // Water Controls
    bool        can_boil_water      = false;

    // Acid Controls
    float       corrosion_resist    = 0.8f;
    bool        is_corrosion_source = false; // Can this pixel type corrode others?

    // Fire Controls
    float       flammability        = 0.0f; // Chance that is_burning = true from neighbour
    float       put_out_surrounded  = 0.0f; // Chance that is_burning = false if surrounded
    float       put_out             = 0.0f; // Chance that is_burning = false otherwise
    float       burn_out_chance     = 0.0f; // Chance that the pixel gets destroyed
    float       explosion_chance    = 0.0f; // Change that it explodes when destroyed
    bool        is_burn_source      = false; // Can this pixel cause others to burn?
    bool        is_ember_source     = false; // Does this pixel produce embers?

    // Electricity Controls
    bool        is_power_source     = false;
    bool        is_conductor        = false;
    int         power_max_level     = 0; // Power level set when the pixel turns on
    int         power_min_level     = 0; // Under this level, the pixel is no longer powered
};

struct pixel
{
    pixel_type      type;
    glm::vec4       colour;
    glm::vec2       velocity;
    std::bitset<64> flags;
    std::int8_t     power = 0;

    static auto air() -> pixel;
    static auto sand() -> pixel;
    static auto coal() -> pixel;
    static auto dirt() -> pixel;
    static auto rock() -> pixel;
    static auto water() -> pixel;
    static auto lava() -> pixel;
    static auto acid() -> pixel;
    static auto steam() -> pixel;
    static auto titanium() -> pixel;
    static auto fuse() -> pixel;
    static auto ember() -> pixel;
    static auto oil() -> pixel;
    static auto gunpowder() -> pixel;
    static auto methane() -> pixel;
    static auto battery() -> pixel;
    static auto solder() -> pixel;
};

auto properties(const pixel& px) -> const pixel_properties&;

auto serialise(auto& archive, pixel& px) -> void {
    archive(px.type, px.colour, px.velocity, px.flags);
}

auto is_powered(const pixel& px) -> bool;

}