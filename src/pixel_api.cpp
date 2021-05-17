#include "pixel_api.h"

namespace alc {
namespace {

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + alc::tile::SIZE * pos.y;
}

}

glm::ivec2 pixel_api::move_to(glm::ivec2 offset)
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

    glm::ivec2 position = d_pos;

    auto a = d_pos;
    auto b = d_pos + offset;
    int steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        int x = a.x + (float)(i + 1)/steps * (b.x - a.x);
        int y = a.y + (float)(i + 1)/steps * (b.y - a.y);
        glm::ivec2 p{x, y};
        if (!tile::valid(p)) { break; }

        auto curr_pos = get_pos(position);
        auto next_pos = get_pos(p);
        if (can_displace(d_pixels_ref[curr_pos], d_pixels_ref[next_pos])) {
            std::swap(d_pixels_ref[curr_pos], d_pixels_ref[next_pos]);
            curr_pos = next_pos;
            position = p;
        } else {
            break;
        }
    }
    if (position != d_pos) {
        d_pixels_ref[get_pos(position)].updated_this_frame = true;
    }
    return position - d_pos;
}

pixel& pixel_api::get(glm::ivec2 offset)
{
    return d_pixels_ref[get_pos(d_pos + offset)];
}

bool pixel_api::valid(glm::ivec2 offset)
{
    return tile::valid(d_pos + offset);
}

}