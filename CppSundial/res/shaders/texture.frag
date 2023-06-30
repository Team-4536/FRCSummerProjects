#version 330 core

out vec4 color;

in vec4 gl_FragCoord;
in vec2 vUv;

uniform sampler2D uTexture;

void main()
{
    vec4 texColor = texture(uTexture, vUv);
    color = texColor;

    if (texColor.a <= 0.0) { discard; }
};
