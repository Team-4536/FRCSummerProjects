#version 460

struct lineVert {
    vec4 pos;
    vec4 color;
};

layout(location = 0) in vec2 position;
layout(std430, binding = 0) buffer vertBuffer {
    lineVert verts[];
};

uniform mat4 uVP;
uniform vec2 uResolution;
uniform float uThickness;

out vec4 vColor;

// STOLEN FROM HERE: https://stackoverflow.com/questions/60440682/drawing-a-line-in-modern-opengl
void main()
{
    int line_i = gl_VertexID / 6;
    int tri_i  = gl_VertexID % 6;

    vColor = verts[line_i].color;

    vec4 va[4];
    for (int i=0; i<4; ++i)
    {
        va[i] = uVP * verts[line_i+i].pos;
        va[i].xyz /= va[i].w;
        va[i].xy = (va[i].xy + 1.0) * 0.5 * uResolution;
    }

    vec2 v_line  = normalize(va[2].xy - va[1].xy);
    vec2 nv_line = vec2(-v_line.y, v_line.x);

    vec4 pos;
    if (tri_i == 0 || tri_i == 1 || tri_i == 3)
    {
        vec2 v_pred  = normalize(va[1].xy - va[0].xy);
        vec2 v_miter = normalize(nv_line + vec2(-v_pred.y, v_pred.x));

        pos = va[1];
        pos.xy += v_miter * uThickness * (tri_i == 1 ? -0.5 : 0.5) / dot(v_miter, nv_line);
    }
    else
    {
        vec2 v_succ  = normalize(va[3].xy - va[2].xy);
        vec2 v_miter = normalize(nv_line + vec2(-v_succ.y, v_succ.x));

        pos = va[2];
        pos.xy += v_miter * uThickness * (tri_i == 5 ? 0.5 : -0.5) / dot(v_miter, nv_line);
    }

    pos.xy = pos.xy / uResolution * 2.0 - 1.0;
    pos.xyz *= pos.w;
    gl_Position = pos;
};