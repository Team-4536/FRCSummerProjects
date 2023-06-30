#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

uniform mat4 uVP;
uniform mat4 uModel;
uniform vec2 uSubStart;
uniform vec2 uSubSize;

out vec2 vUv;

void main()
{
    vUv = vec2(uv.x * uSubSize.x, uv.y * uSubSize.y) + uSubStart;
    gl_Position = uVP * uModel * vec4(position.x, position.y, position.z, 1);
};


