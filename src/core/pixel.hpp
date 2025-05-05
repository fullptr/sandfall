#pragma once
#include <glm/glm.hpp>
#include <box2d/box2d.h>

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

enum class pixel_power_type : std::uint8_t
{
    none,
    source,
    conductor
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
    solder,
    diode_in,
    diode_out,
    spark,
    c4,
    relay
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

    // Misc
    float       spontaneous_destroy = 0.0f; // Chance the pixel randomly dies

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
    bool        explodes_on_power   = false; // Does this pixel explode when powered?

    // Electricity Controls
    pixel_power_type power_type     = pixel_power_type::none;
    std::uint8_t     power_max      = 0; // The maximum power this pixel can accept
};

struct pixel
{
    pixel_type      type;
    glm::vec4       colour;
    glm::vec2       velocity;
    std::bitset<64> flags;

    // For conductors, this is the current power level
    // For power sources, it is a value between in [0, 5), with 5 being active
    std::uint8_t    power = 0;

    static auto air()       -> pixel;
    static auto sand()      -> pixel;
    static auto coal()      -> pixel;
    static auto dirt()      -> pixel;
    static auto rock()      -> pixel;
    static auto water()     -> pixel;
    static auto lava()      -> pixel;
    static auto acid()      -> pixel;
    static auto steam()     -> pixel;
    static auto titanium()  -> pixel;
    static auto fuse()      -> pixel;
    static auto ember()     -> pixel;
    static auto oil()       -> pixel;
    static auto gunpowder() -> pixel;
    static auto methane()   -> pixel;
    static auto battery()   -> pixel;
    static auto solder()    -> pixel;
    static auto diode_in()  -> pixel;
    static auto diode_out() -> pixel;
    static auto spark()     -> pixel;
    static auto c4()        -> pixel;
    static auto relay()     -> pixel;
};

auto properties(const pixel& px) -> const pixel_properties&;

auto serialise(auto& archive, pixel& px) -> void {
    archive(px.type, px.colour, px.velocity, px.flags, px.power);
}

auto is_active_power_source(const pixel& px) -> bool;

}