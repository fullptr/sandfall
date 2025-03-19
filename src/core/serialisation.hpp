#pragma once
#include "world.hpp"

#include <memory>
#include <string>

namespace sand {

auto new_level(int chunks_width, int chunks_height) -> std::unique_ptr<sand::level>;
auto save_level(const std::string& file_path, const sand::level& w) -> void;
auto load_level(const std::string& file_path) -> std::unique_ptr<sand::level>;

}