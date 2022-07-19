#pragma once
#include <glm/glm.hpp>

#include <bitset>
#include <cstdint>

namespace sand {

struct pixel;
using affect_neighbour_func = void(*)(pixel& me, pixel& them);

enum class pixel_movement : std::uint8_t
{
    none,
    immovable_solid,
    movable_solid,
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
    ember
};

struct pixel_properties
{
    // Movement Controls
    pixel_movement movement            = pixel_movement::none;
    float          inertial_resistance = 0.0f;
    float          horizontal_transfer = 0.0f;
    int            dispersion_rate     = 0;

    // Water Controls
    bool           can_boil_water      = false;

    // Acid Controls
    float          corrosion_resist    = 0.8f;
    bool           is_corrosion_source = false; // Can this pixel type corrode others?

    // Fire Controls
    float          flammability        = 0.0f; // Chance that is_burning = true from neighbour
    float          put_out_surrounded  = 0.0f; // Chance that is_burning = false if surrounded
    float          put_out             = 0.0f; // Chance that is_burning = false otherwise
    float          burn_out_chance     = 0.0f; // Chance that the pixel gets destroyed
    bool           is_burn_source      = false; // Can this pixel cause others to burn?
    bool           is_ember_source     = false; // Does this pixel produce embers?

    // Called on each of the pixels neighers (TODO: Remove, rely only on property specific logic)
    affect_neighbour_func affect_neighbour = [](pixel& me, pixel& them) {};
};

struct pixel
{
    pixel_type type;

    glm::vec4 colour;
    glm::vec2 velocity   = {0.0, 0.0};

    bool      is_falling = false;
    bool      is_updated = false;
    bool      is_burning = false;

    auto properties() const -> const pixel_properties&;

    auto serialise(auto& archive) -> void {
        archive(
            type,
            colour,
            velocity,
            is_falling,
            is_updated,
            is_burning
        );
    }

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
};

}