#version 410 core
layout (location = 0) out vec4 out_colour;

in vec2 pass_uv;

void main()
{
    out_colour = vec4(pass_uv, 1.0, 1.0);
}