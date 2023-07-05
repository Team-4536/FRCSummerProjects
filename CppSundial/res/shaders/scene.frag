#version 330 core

out vec4 color;

in vec4 gl_FragCoord;
in vec2 vUv;


uniform vec4 uColor;

void main()
{
    color = uColor;
};
