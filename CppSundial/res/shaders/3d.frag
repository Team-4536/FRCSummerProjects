#version 330 core

out vec4 color;

in vec4 gl_FragCoord;
in vec2 vUv;


uniform vec4 uColor;
uniform sampler2D uTexture;

void main()
{
    color = uColor * texture(uTexture, vUv);
};
