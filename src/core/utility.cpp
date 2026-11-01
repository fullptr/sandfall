#include "utility.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "window.hpp"

#include <array>
#include <bit>
#include <random>
#include <numbers>
#include <iostream>

#include <Windows.h>

namespace sand {
namespace {

// PCG32 (pcg-random.org): small, fast, and good quality in every output bit. Satisfies
// UniformRandomBitGenerator so it also works with the std distributions.
class pcg32
{
    static constexpr u64 multiplier = 6364136223846793005ull;
    static constexpr u64 increment  = 1442695040888963407ull;

    u64 d_state = 0;

public:
    using result_type = u32;

    constexpr explicit pcg32(u64 seed)
    {
        (*this)();
        d_state += seed;
        (*this)();
    }

    // Parenthesised so the min/max macros from Windows.h don't expand
    static constexpr auto (min)() -> result_type { return 0; }
    static constexpr auto (max)() -> result_type { return (std::numeric_limits<u32>::max)(); }

    constexpr auto operator()() -> result_type
    {
        const auto old = d_state;
        d_state = old * multiplier + increment;
        const auto xorshifted = static_cast<u32>(((old >> 18u) ^ old) >> 27u);
        return std::rotr(xorshifted, static_cast<int>(old >> 59u));
    }
};

// One generator for the whole game. Fixed seed, so runs are reproducible given the same inputs.
// Not thread safe; give each thread its own if the simulation becomes multithreaded.
auto rng = pcg32{0x853c49e6748fea9bull};

}

timer::timer()
    : d_clock()
    , d_prev_time(d_clock.now())
    , d_curr_time(d_prev_time)
    , d_last_time_printed(d_prev_time)
    , d_frame_count(0)
{}

auto timer::on_update() -> double
{
    d_prev_time = d_curr_time;
    d_curr_time = d_clock.now();
    ++d_frame_count;

    if (d_curr_time - d_last_time_printed >= std::chrono::seconds(1)) {
        d_frame_rate = d_frame_count;
        d_frame_count = 0;
        d_last_time_printed = d_curr_time;
    }

    const auto dt = std::chrono::duration<double>{d_curr_time - d_prev_time};
    return dt.count();
}

auto timer::now() const -> clock::time_point
{
    return d_clock.now();
}

auto random_from_range(float min, float max) -> float
{
    return min + (max - min) * random_unit();
}

auto random_from_range(int min, int max) -> int
{
    // Maps a 32-bit value onto the inclusive range with a multiply instead of a modulo
    // (Lemire's method). The bias is negligible for ranges this small.
    const auto range = static_cast<u64>(static_cast<i64>(max) - min + 1);
    return min + static_cast<int>((static_cast<u64>(rng()) * range) >> 32);
}

auto random_normal(float centre, float sd) -> float
{
    return std::normal_distribution(centre, sd)(rng);
}

auto random_from_circle(float radius) -> glm::ivec2
{
    const auto r = random_from_range(0.0f, radius);
    const auto x = random_from_range(0.0f, 2.0f * std::numbers::pi);
    return { r * std::cos(x), r * std::sin(x) };
}

auto coin_flip() -> bool
{
    return rng() >> 31;
}

auto sign_flip() -> int
{
    return coin_flip() ? 1 : -1;
}

auto random_unit() -> float
{
    // The top 24 bits exactly fill a float's mantissa, giving a uniform value in [0, 1)
    return static_cast<float>(rng() >> 8) * 0x1.0p-24f;
}

auto _print_inner(const std::string& msg) -> void
{
    std::cout << msg;
}

auto get_executable_filepath() -> std::filesystem::path
{
    auto buffer = std::vector<char>{};
    buffer.resize(16);
    while (true) {
        const auto rc = GetModuleFileNameA(nullptr, buffer.data(), buffer.size());
        if (rc < buffer.size()) {
            return std::filesystem::path{buffer.data()};
        }
        buffer.resize(2 * buffer.size());
    }
}

auto mouse_pos_world_space(const input& in, const sand::camera& c) -> glm::vec2
{
    return in.position() / c.world_to_screen + c.top_left;
}

auto pixel_at_mouse(const input& in, const sand::camera& c) -> pixel_pos
{
    const auto p = glm::ivec2{mouse_pos_world_space(in, c)};
    return {p.x, p.y};
}

auto pixel_to_physics(glm::vec2 px) -> b2Vec2
{
    b2Vec2 pos(static_cast<float>(px.x) / sand::config::pixels_per_meter, static_cast<float>(px.y) / sand::config::pixels_per_meter);
    return pos;
}

auto pixel_to_physics(pixel_pos px) -> b2Vec2
{
    b2Vec2 pos(static_cast<float>(px.x) / sand::config::pixels_per_meter, static_cast<float>(px.y) / sand::config::pixels_per_meter);
    return pos;
}

auto pixel_to_physics(float px) -> float
{
    return px / sand::config::pixels_per_meter;
}

// Converts a point in world space to pixel space
auto physics_to_pixel(b2Vec2 px) -> glm::vec2
{
    glm::vec2 pos(px.x * sand::config::pixels_per_meter, px.y * sand::config::pixels_per_meter);
    return pos;
}

auto physics_to_pixel(float px) -> float
{
    return px * sand::config::pixels_per_meter;
}
    
}