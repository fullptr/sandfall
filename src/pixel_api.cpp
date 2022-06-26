#include "pixel_api.h"

namespace sand {
namespace {

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + tile::SIZE * pos.y;
}

}

bool pixel_api::move_to(glm::ivec2 offset)
{
    return move_towards(d_pixels_ref, d_pos, offset);
}

auto move_towards(tile::pixels& pixels, glm::ivec2 from, glm::ivec2 offset) -> bool
{
    const auto can_displace = [](const pixel& src, const pixel& dst) {
        if (src.type == pixel_type::sand && (dst.type == pixel_type::air || dst.type == pixel_type::water)) {
            return true;
        }
        else if (src.type == pixel_type::water && dst.type == pixel_type::air) {
            return true;
        }
        return false;
    };

    glm::ivec2 position = from;

    auto a = from;
    auto b = from + offset;
    int steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        int x = a.x + (float)(i + 1)/steps * (b.x - a.x);
        int y = a.y + (float)(i + 1)/steps * (b.y - a.y);
        glm::ivec2 p{x, y};
        if (!tile::valid(p)) { break; }

        auto curr_pos = get_pos(position);
        auto next_pos = get_pos(p);
        if (can_displace(pixels[curr_pos], pixels[next_pos])) {
            std::swap(pixels[curr_pos], pixels[next_pos]);
            curr_pos = next_pos;
            position = p;
        } else {
            break;
        }
    }
    if (position != from) {
        pixels[get_pos(position)].updated_this_frame = true;
    }
    return position != from;
}

}