#ifndef INCLUDED_ALCHIMIA_PIXEL_API
#define INCLUDED_ALCHIMIA_PIXEL_API
#include "tile.h"

#include <glm/glm.hpp>

namespace alc {

class pixel_api
// A light wrapper around pixel data to make the material update functions simpler.
{
    
    tile::pixels& d_pixels_ref;
    glm::ivec2    d_pos;

public:
    pixel_api(tile::pixels& pixels_ref, glm::ivec2 pos) : d_pixels_ref(pixels_ref), d_pos(pos) {}

    bool move(glm::ivec2 offset);
    pixel& get(glm::ivec2 offset);
    bool valid(glm::ivec2 offset);
};

}

#endif // INCLUDED_ALCHIMIA_PIXEL_API