#pragma once
#include <glm/glm.hpp>

#include <variant>

namespace sand {

struct movable_solid
{
    // Runtime values
    glm::vec2 velocity;
    bool      is_falling;

    // Static values
    float     inertial_resistance;
    float     horizontal_transfer;
};

struct static_solid
{
};

struct liquid
{
    // Runtime values
    glm::vec2 velocity;

    // Static values
    int       dispersion_rate;
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

    static auto air() -> pixel;
    static auto sand() -> pixel;
    static auto coal() -> pixel;
    static auto dirt() -> pixel;
    static auto rock() -> pixel;
    static auto water() -> pixel;
    static auto lava() -> pixel;
};

}