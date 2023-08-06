#version 330 core

layout(location = 0) in vec2 position;

uniform mat4 uVP;

void main()
{
    gl_Position = uVP * vec4(position, 0, 1);
};