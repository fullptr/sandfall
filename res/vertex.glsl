#version 410 core
layout (location = 0) in vec4 position_uv;

uniform mat4 u_proj_matrix;

out vec2 pass_uv;

void main()
{
    vec2 position = position_uv.xy;
    pass_uv = position_uv.zw;
    gl_Position = u_proj_matrix * vec4(position, 0, 1);
}