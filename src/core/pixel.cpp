#include "pixel.hpp"
#include "utility.hpp"

#include <cstdlib>
#include <print>
#include <vector>

namespace sand {
namespace {

auto light_noise() -> glm::vec4
{
    return {
        random_from_range(-0.04f, 0.04f),
        random_from_range(-0.04f, 0.04f),
        random_from_range(-0.04f, 0.04f),
        1.0f
    };
}

}

auto properties(const pixel& pix) -> const pixel_properties&
{
    switch (pix.type) {
        case pixel_type::none: {
            static constexpr auto px = pixel_properties{
                .phase = pixel_phase::gas,
                .corrosion_resist = 1.0f
            };
            return px;
        }
        case pixel_type::sand: {
            static constexpr auto px = pixel_properties{
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.1f,
                .corrosion_resist = 0.3f
            };
            return px;
        }
        case pixel_type::dirt: {
            static constexpr auto px = pixel_properties{
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.4f,
                .corrosion_resist = 0.5f
            };
            return px;
        }
        case pixel_type::coal: {
            static constexpr auto px = pixel_properties{
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.95f,
                .corrosion_resist = 0.8f,
                .flammability = 0.02f,
                .put_out_surrounded = 0.15f,
                .put_out = 0.02f,
                .burn_out_chance = 0.005f
            };
            return px;
        }
        case pixel_type::water: {
            static constexpr auto px = pixel_properties{
                .phase = pixel_phase::liquid,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .dispersion_rate = 5,
                .corrosion_resist = 1.0f,
            };
            return px;
        }
        case pixel_type::lava: {
            static constexpr auto px = pixel_properties{
                .phase = pixel_phase::liquid,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .dispersion_rate = 1,
                .can_boil_water = true,
                .corrosion_resist = 1.0f,
                .is_burn_source = true,
                .is_ember_source = true
            };
            return px;
        }
        case pixel_type::acid: {
            static constexpr auto px = pixel_properties{
                .phase = pixel_phase::liquid,
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .dispersion_rate = 1,
                .corrosion_resist = 1.0f,
                .is_corrosion_source = true
            };
            return px;
        }
        case pixel_type::rock: {
            static constexpr auto px = pixel_properties{
                .corrosion_resist = 0.95f,
            };
            return px;
        }
        case pixel_type::titanium: {
            static constexpr auto px = pixel_properties{
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::conductor,
                .power_max = 25
            };
            return px;
        }
        case pixel_type::steam: {
            static constexpr auto px = pixel_properties{
                .phase = pixel_phase::gas,
                .can_move_diagonally = true,
                .gravity_factor = -1.0f,
                .dispersion_rate = 9,
                .corrosion_resist = 0.0f
            };
            return px;
        }
        case pixel_type::fuse: {
            static constexpr auto px = pixel_properties{
                .corrosion_resist = 0.1f,
                .flammability = 0.25f,
                .put_out_surrounded = 0.0f,
                .put_out = 0.0f,
                .burn_out_chance = 0.1f
            };
            return px;
        }
        case pixel_type::ember: {
            static constexpr auto px = pixel_properties{
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
            return px;
        }
        case pixel_type::oil: {
            static constexpr auto px = pixel_properties{
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
            return px;
        }
        case pixel_type::gunpowder: {
            static constexpr auto px = pixel_properties{
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
            return px;
        }
        case pixel_type::methane: {
            static constexpr auto px = pixel_properties{
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
            return px;
        }
        case pixel_type::battery: {
            static constexpr auto px = pixel_properties{
                .always_awake = true,
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::source,
                .power_max = 5
            };
            return px;
        }
        case pixel_type::solder: {
            static constexpr auto px = pixel_properties{
                .can_move_diagonally = true,
                .gravity_factor = 1.0f,
                .inertial_resistance = 0.05f,
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::conductor,
                .power_max = 24
            };
            return px;
        }
        case pixel_type::diode_in:
        case pixel_type::diode_out: {
            static constexpr auto px = pixel_properties{
                .corrosion_resist = 1.0f,
                .power_type = pixel_power_type::conductor,
                .power_max = 25
            };
            return px;
        }
        case pixel_type::spark: {
            static constexpr auto px = pixel_properties{
                .always_awake = true,
                .spontaneous_destroy = 0.3f,
                .corrosion_resist = 0.1f,
                .power_type = pixel_power_type::source,
                .power_max = 100
            };
            return px;
        }
        case pixel_type::c4: {
            static constexpr auto px = pixel_properties{
                .corrosion_resist = 0.95f,
                .explodes_on_power = true,
                .power_type = pixel_power_type::conductor,
                .power_max = 10
            };
            return px;
        }
        case pixel_type::relay: {
            static constexpr auto px = pixel_properties{
                .corrosion_resist = 1.0f
            };
            return px;
        }
        default: {
            std::print("ERROR: Unknown pixel type {}\n", static_cast<int>(pix.type));
            static constexpr auto px = pixel_properties{};
            return px;
        }
    }
}

auto pixel::air() -> pixel
{
    return pixel{
        .type = pixel_type::none,
        .colour = {0.0f, 0.0f, 0.0f, 0.0f}
    };
}

auto pixel::sand() -> pixel
{
    auto p = pixel{
        .type = pixel_type::sand,
        .colour = from_hex(0xF8EFBA) + light_noise()
    };
    p.flags[is_falling] = true;
    return p;
}

auto pixel::coal() -> pixel
{
    auto p = pixel{
        .type = pixel_type::coal,
        .colour = from_hex(0x1E272E) + light_noise()
    };
    p.flags[is_falling] = true;
    return p;
}

auto pixel::dirt() -> pixel
{
    auto p = pixel{
        .type = pixel_type::dirt,
        .colour = from_hex(0x5C1D06) + light_noise()
    };
    p.flags[is_falling] = true;
    return p;
}

auto pixel::rock() -> pixel
{
    return {
        .type = pixel_type::rock,
        .colour = from_hex(0xC8C8C8) + light_noise()
    };
}

auto pixel::water() -> pixel
{
    return {
        .type = pixel_type::water,
        .colour = from_hex(0x1B9CFC) + light_noise()
    };
}

auto pixel::lava() -> pixel
{
    return {
        .type = pixel_type::lava,
        .colour = from_hex(0xF97F51) + light_noise()
    };
}

auto pixel::acid() -> pixel
{
    return {
        .type = pixel_type::acid,
        .colour = from_hex(0x2ED573) + light_noise()
    };
}

auto pixel::steam() -> pixel
{
    return {
        .type = pixel_type::steam,
        .colour = from_hex(0x9AECDB) + light_noise()
    };
}

auto pixel::titanium() -> pixel
{
    return {
        .type = pixel_type::titanium,
        .colour = from_hex(0xDFE4EA)
    };
}

auto pixel::fuse() -> pixel
{
    return {
        .type = pixel_type::fuse,
        .colour = from_hex(0x45AAF2) + light_noise()
    };
}

auto pixel::ember() -> pixel
{
    auto p = pixel{
        .type = pixel_type::ember,
        .colour = from_hex(0xFFFFFF)
    };
    p.flags[is_burning] = true;
    return p;
}

auto pixel::oil() -> pixel
{
    return {
        .type = pixel_type::oil,
        .colour = from_hex(0x650C30) + light_noise()
    };
}

auto pixel::gunpowder() -> pixel
{
    auto p = pixel{
        .type = pixel_type::gunpowder,
        .colour = from_hex(0x485460) + light_noise()
    };
    p.flags[is_falling] = true;
    return p;
}

auto pixel::methane() -> pixel
{
    return {
        .type = pixel_type::methane,
        .colour = from_hex(0xCED6E0) + light_noise()
    };
}

auto pixel::battery() -> pixel
{
    return {
        .type = pixel_type::battery,
        .colour = from_hex(0xF0932B)
    };
}

auto pixel::solder() -> pixel
{
    auto p = pixel{
        .type = pixel_type::solder,
        .colour = from_hex(0xB2BEC3)
    };
    p.flags[is_falling] = true;
    return p;
}

auto pixel::diode_in() -> pixel
{
    return {
        .type = pixel_type::diode_in,
        .colour = from_hex(0x22A6B3)
    };
}

auto pixel::diode_out() -> pixel
{
    return {
        .type = pixel_type::diode_out,
        .colour = from_hex(0xBE2EDD)
    };
}

auto pixel::spark() -> pixel
{
    auto p = pixel{
        .type = pixel_type::spark,
        .colour = from_hex(0xE1B12C)
    };
    p.power = properties(p).power_max;
    return p;
}

auto pixel::c4() -> pixel
{
    return {
        .type = pixel_type::c4,
        .colour = from_hex(0xB8E994)
    };
}

auto pixel::relay() -> pixel
{
    return {
        .type = pixel_type::relay,
        .colour = from_hex(0x192A56)
    };
}

auto is_active_power_source(const pixel& px) -> bool
{
    const auto& props = properties(px);
    return props.power_type == pixel_power_type::source && px.power == props.power_max;
}

}