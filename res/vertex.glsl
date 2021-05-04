#version 410 core
layout (location = 0) in vec2 position;

uniform mat4 u_proj_matrix;

void main()
{
    gl_Position = u_proj_matrix * vec4(position, 0, 1);
}