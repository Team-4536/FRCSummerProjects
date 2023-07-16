#version 330 core

out vec4 color;

layout(origin_upper_left) in vec4 gl_FragCoord;

in vec2 vUv;
in vec2 vCenter;
in vec2 vHalfSize;


uniform vec4 uColor;
uniform sampler2D uTexture;
uniform sampler2D uFontTexture;

uniform vec2 uClipStart;
uniform vec2 uClipEnd;
uniform float uCornerRadius;



float roundedRectSDF(vec2 sample_pos,
                     vec2 rect_center,
                     vec2 rect_half_size,
                     float r)
{
  vec2 d2 = (abs(rect_center - sample_pos) -
             rect_half_size +
             vec2(r, r));
  return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}


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

    float sdfOut = (roundedRectSDF(gl_FragCoord.xy, vCenter, vHalfSize, uCornerRadius)<0)? 1 : 0;
    vec4 sdf = vec4(1, 1, 1, sdfOut);

    color = uColor * texColor * fontColor * sdf;

    if (color.a <= 0.01) { discard; }

    // probably bad for performance
    if     (gl_FragCoord.x > uClipEnd.x) { discard; }
    else if(gl_FragCoord.y > uClipEnd.y) { discard; }
    else if(gl_FragCoord.x <= uClipStart.x) { discard; }
    else if(gl_FragCoord.y <= uClipStart.y) { discard; }
};
