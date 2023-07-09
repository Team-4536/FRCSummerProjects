#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

uniform mat4 uVP;
uniform mat4 uModel;

out vec2 vUv;

void main()
{
    gl_Position = uVP * uModel * vec4(position, 1);
    vUv = uv;
};


