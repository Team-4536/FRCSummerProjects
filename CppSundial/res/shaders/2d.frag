
#version 330 core

out vec4 color;

in vec2 vUv;

uniform sampler2D uTexture;

void main()
{
    color = texture(uTexture, vUv);
    if (color.a <= 0.01) { discard; }
};
