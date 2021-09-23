#version 450 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 tex;


out vec2 pos;

uniform float uGAME_W;
uniform float uGAME_H;

out float GAME_W;
out float GAME_H;

void main()
{
    // transform the vertex position
    gl_Position = gl_ModelViewProjectionMatrix * pos;

    // transform the texture coordinates
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    // forward the vertex color
    gl_FrontColor = gl_Color;

    pos = gl_Vertex.xy;

    GAME_W = uGAME_W;
    GAME_H = uGAME_H;
}
