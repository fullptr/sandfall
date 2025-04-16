#pragma once
#include "buffer.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <glm/glm.hpp>

namespace sand {

class ui
{
public:
    ui();
    void start_frame();
    void end_frame();

    bool button(glm::vec2 pos, float width, float height);
};

}