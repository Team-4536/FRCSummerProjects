#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv; // CLEANUP: remove from vert defs bc it's generated


in int gl_VertexID;

out vec2 vUv;


uniform mat4 uVP;
uniform vec2 uDstStart;
uniform vec2 uDstEnd;
uniform vec2 uSrcStart;
uniform vec2 uSrcEnd;


vec2 cornerTable[4] = vec2[](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1),
    vec2(1, 0)
    );


void main() {

    vec2 corner = cornerTable[gl_VertexID];
    corner *= uDstEnd - uDstStart;
    corner += uDstStart;

    vUv = cornerTable[gl_VertexID];
    vUv *= uSrcEnd - uSrcStart;
    vUv += uSrcStart;

    gl_Position = uVP * vec4(corner, 0, 1);
}