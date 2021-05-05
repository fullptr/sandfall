#version 410 core
layout (location = 0) out vec4 out_colour;

in vec2 pass_uv;

uniform sampler2D u_texture;

void main()
{
    //out_colour = vec4(pass_uv, 1.0, 1.0);
    out_colour = texture(u_texture, pass_uv);
}