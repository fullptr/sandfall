#pragma once
#include <glm/glm.hpp>

#include <variant>

namespace sand {

struct movable_solid
{
    // Runtime values
    glm::vec2 velocity;
    bool      is_falling;

    // Static values - these define how the element behaves and should stay const
    float     intertial_resistance;
};

struct static_solid
{
};

struct liquid
{
    // Runtime values
    glm::vec2 velocity        = {0.0, 0.0};

    // Static values - these define how the element behaves and should stay const
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
    static pixel coal();
    static pixel rock();
    static pixel water();
    static pixel red_sand();
};

}