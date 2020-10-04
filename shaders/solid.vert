#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec3 a_normal;

uniform mat4 transform;

out vec3 color;
out vec3 normal;

void main()
{
    color = a_color;
    normal = a_normal;
    gl_Position = transform * vec4(a_position, 1.0);
}
