#include "serialisation.hpp"
#include "world_save.hpp"
#include "world.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <memory>

#include <cereal/archives/binary.hpp>

namespace sand {

auto new_level(int chunks_width, int chunks_height) -> std::unique_ptr<sand::level>
{
    const auto width = sand::config::chunk_size * chunks_width;
    const auto height = sand::config::chunk_size * chunks_height;
    return std::make_unique<sand::level>(
        width,
        height,
        std::vector<sand::pixel>(width * height, sand::pixel::air())
    );
}

auto save_level(const std::string& file_path, const sand::level& w) -> void
{
    auto file = std::ofstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryOutputArchive{file};

    auto save = sand::world_save{
        .pixels = w.pixels.pixels(),
        .width = w.pixels.width(),
        .height = w.pixels.height(),
        .spawn_point = {w.spawn_point.x, w.spawn_point.y}
    };

    archive(save);
}

auto load_level(const std::string& file_path) -> std::unique_ptr<sand::level>
{
    auto file = std::ifstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryInputArchive{file};

    auto save = sand::world_save{};
    archive(save);

    auto w = std::make_unique<sand::level>(save.width, save.height, save.pixels);
    w->spawn_point = {save.spawn_point.x, save.spawn_point.y};
    w->player.set_position(w->spawn_point);
    return w;
}

}