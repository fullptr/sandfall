#pragma once
#include "utility.hpp"

#include <glm/glm.hpp>
#include <glm/ext/vector_uint4_sized.hpp>
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

// Saves store these as numbers, so only ever add new types at the end
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

// Keep in sync with the last entry of pixel_type
inline constexpr std::size_t num_pixel_types = static_cast<std::size_t>(pixel_type::relay) + 1;

// Everything about a type of pixel: how it looks, the state new pixels start in, and
// how it behaves in the simulation
struct material
{
    // Appearance
    const char* name                = "";
    glm::u8vec4 colour              = {0, 0, 0, 0};
    bool        colour_noise        = false; // Vary each pixel's colour slightly

    // Starting State
    bool        starts_falling      = false;
    bool        starts_burning      = false;
    bool        starts_powered      = false; // Starts with power_max power

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
    glm::u8vec4     colour;
    glm::vec2       velocity;
    std::bitset<64> flags;

    // For conductors, this is the current power level
    // For power sources, it is a value between in [0, 5), with 5 being active
    std::uint8_t    power = 0;
};

auto material_of(pixel_type type) -> const material&;

inline auto material_of(const pixel& px) -> const material&
{
    return material_of(px.type);
}

// Creates a new pixel of the given type, as described by its material
auto make_pixel(pixel_type type) -> pixel;

// Colours are saved as float vec4s, from before pixels stored them as bytes. Convert
// on the way in and out to keep existing saves loadable.
auto save(auto& archive, const pixel& px) -> void
{
    archive(px.type, to_float_colour(px.colour), px.velocity, px.flags, px.power);
}

auto load(auto& archive, pixel& px) -> void
{
    auto colour = glm::vec4{};
    archive(px.type, colour, px.velocity, px.flags, px.power);
    px.colour = to_byte_colour(colour);
}

auto is_active_power_source(const pixel& px) -> bool;

}