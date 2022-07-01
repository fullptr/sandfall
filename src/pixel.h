#pragma once
#include <glm/glm.hpp>

#include <variant>

namespace sand {

struct movable_solid
{
    glm::vec2 velocity = {0.0, 0.0};
    bool      is_falling = true;
    float     intertial_resistance = 0.1f;
};

struct static_solid
{
};

struct liquid
{
    glm::vec2 velocity        = {0.0, 0.0};
    int       dispersion_rate = 3;
};

struct gas
{
};

using empty = std::monostate;

using pixel_data = std::variant<
    movable_solid,
    static_solid,
    liquid,
    gas,
    empty
>;

struct pixel
{
    pixel_data data;
    glm::vec4  colour;
    bool       updated_this_frame = false;

    template <typename... Ts>
    auto is() const -> bool { return (std::holds_alternative<Ts>(data) || ...); }

    template <typename T>
    auto as() -> T& { return std::get<T>(data); }

    template <typename T>
    auto as() const -> const T& { return std::get<T>(data); }

    // TODO: Move out of struct
    static pixel air();
    static pixel sand();
    static pixel rock();
    static pixel water();
    static pixel red_sand();
};

}