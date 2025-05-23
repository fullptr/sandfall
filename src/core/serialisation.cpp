#include "serialisation.hpp"
#include "world_save.hpp"
#include "world.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <memory>

#include <cereal/archives/binary.hpp>

namespace sand {

auto new_level(int chunks_width, int chunks_height) -> level
{
    const auto width = sand::config::chunk_size * chunks_width;
    const auto height = sand::config::chunk_size * chunks_height;
    return {
        pixel_world{width, height},
        physics_world{},
        registry{},
        pixel_pos{width/2, height/2},
        apx::null
    };
}

auto save_level(const std::string& file_path, const sand::level& w) -> void
{
    auto file = std::ofstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryOutputArchive{file};

    auto save = sand::world_save{
        .pixels = w.pixels.pixels(),
        .width = static_cast<std::size_t>(w.pixels.width_in_pixels()),
        .height = static_cast<std::size_t>(w.pixels.height_in_pixels()),
        .spawn_point = {w.spawn_point.x, w.spawn_point.y}
    };

    archive(save);
}

auto load_level(const std::string& file_path) -> level
{
    auto file = std::ifstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryInputArchive{file};

    auto save = sand::world_save{};
    archive(save);

    // TODO: Store the sizes as u32's in the file
    return {
        pixel_world{static_cast<i32>(save.width), static_cast<i32>(save.height), save.pixels},
        physics_world{},
        registry{},
        pixel_pos{save.spawn_point.x, save.spawn_point.y},
        apx::null
    };
}

}