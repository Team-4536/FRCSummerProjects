#version 330 core

out vec4 color;

layout(origin_upper_left) in vec4 gl_FragCoord;

in vec2 vUv;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform sampler2D uFontTexture;

uniform vec2 uClipStart;
uniform vec2 uClipEnd;

void main()
{
    vec2 nUv = vUv;
    nUv.y = 1 - nUv.y;
    // this is awful, but
    // STB uses down positive coords, so the font UVs go UL to BR
    // regular textures are meant to use BL to UR UVs
    // flipping the font pixels would be a pain in the ass so im leaving it like this
    // CLEANUP: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    vec4 texColor = texture(uTexture, nUv);
    vec4 fontColor = vec4(1.0, 1.0, 1.0, texture(uFontTexture, vUv).r);
    color = uColor * texColor * fontColor;

    // color = vec4(gl_FragCoord.x / 100, gl_FragCoord.y / 100, 0, 1);

    // color = vec4(vUv.x, vUv.y, 1.0, 1.0);
    if (color.a <= 0.01) { discard; }

    // probably bad for performance
    if     (gl_FragCoord.x > uClipEnd.x) { discard; }
    else if(gl_FragCoord.y > uClipEnd.y) { discard; }
    else if(gl_FragCoord.x <= uClipStart.x) { discard; }
    else if(gl_FragCoord.y <= uClipStart.y) { discard; }
};
