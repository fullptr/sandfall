#pragma once
#include "input.hpp"
#include "camera.hpp"

namespace sand {

class window;

// This just aggregates all of the key non-level pieces of the code
// needed in the update logic.
struct context
{
    window* window;
    input   input;
    camera  camera;
};

}