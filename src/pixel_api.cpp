#include "pixel_api.h"
#include "generator.h"

namespace alc {
namespace {

alc::generator<glm::ivec2> pixel_path(glm::ivec2 a, glm::ivec2 b)
{
    // The number of steps taken will be the number of pixels in the longest
    // direction. This will ensure no missing pixels.
    int steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        int x = a.x + (float)(i + 1)/steps * (b.x - a.x);
        int y = a.y + (float)(i + 1)/steps * (b.y - a.y);
        co_yield {x, y};
    }
}

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + alc::tile::SIZE * pos.y;
}

}

bool pixel_api::move(glm::ivec2 offset)
{
    const auto can_displace = [](const pixel& src, const pixel& dst) {
        if (src.type == pixel_type::sand && (dst.type == pixel_type::air || dst.type == pixel_type::water)) {
            return !dst.updated_this_frame;
        }
        else if (src.type == pixel_type::water && dst.type == pixel_type::air) {
            return !dst.updated_this_frame;
        }
        return false;
    };

    std::size_t curr_pos = get_pos(d_pos);
    auto start = curr_pos;
    for (auto p : pixel_path(d_pos, d_pos + offset)) {
        auto next_pos = get_pos(p);
        if (can_displace(d_pixels_ref[curr_pos], d_pixels_ref[next_pos])) {
            std::swap(d_pixels_ref[curr_pos], d_pixels_ref[next_pos]);
            curr_pos = next_pos;
        } else {
            break;
        }
    }
    if (curr_pos != start) {
        d_pixels_ref[curr_pos].updated_this_frame = true;
    }
    return curr_pos != start;
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