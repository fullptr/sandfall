#include "pixel.hpp"
#include "utility.hpp"

#include <cassert>
#include <cstdlib>
#include <print>
#include <vector>

namespace sand {
namespace {

// Slightly varies the colour so pixels of the same type don't all look identical
auto with_noise(glm::u8vec4 colour) -> glm::u8vec4
{
    const auto noise = glm::vec4{
        random_from_range(-0.04f, 0.04f),
        random_from_range(-0.04f, 0.04f),
        random_from_range(-0.04f, 0.04f),
        0.0f
    };
    return to_byte_colour(to_float_colour(colour) + noise);
}

}

// No default case, so a pixel_type missing from this switch is a compile error
#pragma warning(push)
#pragma warning(error: 4062) // enumerator in switch of enum is not handled
auto material_of(pixel_type type) -> const material&
{
    switch (type) {
        case pixel_type::none: {
            static constexpr auto m = material{
                .name = "air",
                .colour = {0, 0, 0, 0},
                .phase = pixel_phase::gas,
                .corrosion_resist = 1.0f
            };
            return m;
        }
        case pixel_type::sand: {
            static constexpr auto m = material{
                .name = "sand",
                .colour = from_hex(0xF8EFBA),
                .colour_noise = true,
                .starts_falling = true,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.1f,
                .corrosion_resist = 0.3f
            };
            return m;
        }
        case pixel_type::dirt: {
            static constexpr auto m = material{
                .name = "dirt",
                .colour = from_hex(0x5C1D06),
                .colour_noise = true,
                .starts_falling = true,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.4f,
                .corrosion_resist = 0.5f
            };
            return m;
        }
        case pixel_type::coal: {
            static constexpr auto m = material{
                .name = "coal",
                .colour = from_hex(0x1E272E),
                .colour_noise = true,
                .starts_falling = true,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.95f,
                .corrosion_resist = 0.8f,
                .flammability = 0.02f,
                .put_out_surrounded = 0.15f,
                .put_out = 0.02f,
                .burn_out_chance = 0.005f
            };
            return m;
        }
        case pixel_type::water: {
            static constexpr auto m = material{
                .name = "water",
                .colour = from_hex(0x1B9CFC),
                .colour_noise = true,
                .phase = pixel_phase::liquid,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .dispersion_rate = 5,
                .corrosion_resist = 1.0f,
            };
            return m;
        }
        case pixel_type::lava: {
            static constexpr auto m = material{
                .name = "lava",
                .colour = from_hex(0xF97F51),
                .colour_noise = true,
                .phase = pixel_phase::liquid,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .dispersion_rate = 1,
                .can_boil_water = true,
                .corrosion_resist = 1.0f,
                .is_burn_source = true,
                .is_ember_source = true
            };
            return m;
        }
        case pixel_type::acid: {
            static constexpr auto m = material{
                .name = "acid",
                .colour = from_hex(0x2ED573),
                .colour_noise = true,
                .phase = pixel_phase::liquid,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .dispersion_rate = 1,
                .corrosion_resist = 1.0f,
                .is_corrosion_source = true
            };
            return m;
        }
        case pixel_type::rock: {
            static constexpr auto m = material{
                .name = "rock",
                .colour = from_hex(0xC8C8C8),
                .colour_noise = true,
                .corrosion_resist = 0.95f,
            };
            return m;
        }
        case pixel_type::titanium: {
            static constexpr auto m = material{
                .name = "titanium",
                .colour = from_hex(0xDFE4EA),
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::conductor,
                .power_max = 25
            };
            return m;
        }
        case pixel_type::steam: {
            static constexpr auto m = material{
                .name = "steam",
                .colour = from_hex(0x9AECDB),
                .colour_noise = true,
                .phase = pixel_phase::gas,
                .can_move_diagonally = true,
                .gravity_factor = -1.0f,
                .dispersion_rate = 9,
                .corrosion_resist = 0.0f
            };
            return m;
        }
        case pixel_type::fuse: {
            static constexpr auto m = material{
                .name = "fuse",
                .colour = from_hex(0x45AAF2),
                .colour_noise = true,
                .corrosion_resist = 0.1f,
                .flammability = 0.25f,
                .put_out_surrounded = 0.0f,
                .put_out = 0.0f,
                .burn_out_chance = 0.1f
            };
            return m;
        }
        case pixel_type::ember: {
            static constexpr auto m = material{
                .name = "ember",
                .colour = from_hex(0xFFFFFF),
                .starts_burning = true,
                .phase = pixel_phase::gas,
                .can_move_diagonally = true,
                .gravity_factor = -1.0f,
                .always_awake = true,
                .corrosion_resist = 0.1f,
                .flammability = 1.0f,
                .put_out_surrounded = 0.0f,
                .put_out = 0.0f,
                .burn_out_chance = 0.2f
            };
            return m;
        }
        case pixel_type::oil: {
            static constexpr auto m = material{
                .name = "oil",
                .colour = from_hex(0x650C30),
                .colour_noise = true,
                .phase = pixel_phase::liquid,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .dispersion_rate = 2,
                .corrosion_resist = 0.1f,
                .flammability = 0.05f,
                .put_out_surrounded = 0.3f,
                .put_out = 0.02f,
                .burn_out_chance = 0.005f
            };
            return m;
        }
        case pixel_type::gunpowder: {
            static constexpr auto m = material{
                .name = "gunpowder",
                .colour = from_hex(0x485460),
                .colour_noise = true,
                .starts_falling = true,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.1f,
                .corrosion_resist = 0.1f,
                .flammability = 0.25f,
                .put_out_surrounded = 0.0f,
                .put_out = 0.0f,
                .burn_out_chance = 0.1f,
                .explosion_chance = 0.001f
            };
            return m;
        }
        case pixel_type::methane: {
            static constexpr auto m = material{
                .name = "methane",
                .colour = from_hex(0xCED6E0),
                .colour_noise = true,
                .phase = pixel_phase::gas,
                .can_move_diagonally = true,
                .gravity_factor = -1.0f,
                .dispersion_rate = 4,
                .corrosion_resist = 0.0f,
                .flammability = 0.25f,
                .put_out_surrounded = 0.0f,
                .put_out = 0.0f,
                .burn_out_chance = 0.1f
            };
            return m;
        }
        case pixel_type::battery: {
            static constexpr auto m = material{
                .name = "battery",
                .colour = from_hex(0xF0932B),
                .always_awake = true,
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::source,
                .power_max = 5
            };
            return m;
        }
        case pixel_type::solder: {
            static constexpr auto m = material{
                .name = "solder",
                .colour = from_hex(0xB2BEC3),
                .starts_falling = true,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.05f,
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::conductor,
                .power_max = 24
            };
            return m;
        }
        case pixel_type::diode_in: {
            static constexpr auto m = material{
                .name = "diode_in",
                .colour = from_hex(0x22A6B3),
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::conductor,
                .power_max = 25
            };
            return m;
        }
        case pixel_type::diode_out: {
            static constexpr auto m = material{
                .name = "diode_out",
                .colour = from_hex(0xBE2EDD),
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::conductor,
                .power_max = 25
            };
            return m;
        }
        case pixel_type::spark: {
            static constexpr auto m = material{
                .name = "spark",
                .colour = from_hex(0xE1B12C),
                .starts_powered = true,
                .always_awake = true,
                .spontaneous_destroy = 0.3f,
                .corrosion_resist = 0.1f,
                .power_type = pixel_power_type::source,
                .power_max = 100
            };
            return m;
        }
        case pixel_type::c4: {
            static constexpr auto m = material{
                .name = "c4",
                .colour = from_hex(0xB8E994),
                .corrosion_resist = 0.95f,
                .explodes_on_power = true,
                .power_type = pixel_power_type::conductor,
                .power_max = 10
            };
            return m;
        }
        case pixel_type::relay: {
            static constexpr auto m = material{
                .name = "relay",
                .colour = from_hex(0x192A56),
                .corrosion_resist = 1.0f
            };
            return m;
        }
    }

    // Only reachable if the value is outside of pixel_type, e.g. from a corrupt save
    assert(false && "unknown pixel type");
    std::print("ERROR: Unknown pixel type {}\n", static_cast<int>(type));
    static constexpr auto m = material{};
    return m;
}
#pragma warning(pop)

auto make_pixel(pixel_type type) -> pixel
{
    const auto& m = material_of(type);
    auto p = pixel{
        .type = type,
        .colour = m.colour_noise ? with_noise(m.colour) : m.colour
    };
    p.flags[is_falling] = m.starts_falling;
    p.flags[is_burning] = m.starts_burning;
    if (m.starts_powered) {
        p.power = m.power_max;
    }
    return p;
}

auto is_active_power_source(const pixel& px) -> bool
{
    const auto& m = material_of(px);
    return m.power_type == pixel_power_type::source && px.power == m.power_max;
}

}
