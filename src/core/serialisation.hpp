#pragma once
#include "world.hpp"

#include <memory>
#include <string>

namespace sand {

auto new_level(i32 chunks_width, i32 chunks_height) -> std::unique_ptr<sand::level>;
auto save_level(const std::string& file_path, const sand::level& w) -> void;
auto load_level(const std::string& file_path) -> std::unique_ptr<sand::level>;

}