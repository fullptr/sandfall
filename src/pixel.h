#ifndef INCLUDED_ALCHIMIA_PIXEL
#define INCLUDED_ALCHIMIA_PIXEL
#include <glm/glm.hpp>

namespace alc {

enum class pixel_type
{
    air,
    sand,
    rock,
    water
};

struct pixel
{
    pixel_type type;
    glm::vec4  colour;

    bool updated_this_frame = false;

    static pixel air();
    static pixel sand();
    static pixel rock();
    static pixel water();
};

}

#endif // INCLUDED_ALCHIMIA_PIXEL