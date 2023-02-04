#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textureCoord;
layout (location = 3) in vec4 tangent;

out vec2 passTextureCoord;

void main()
{
    passTextureCoord = textureCoord;
    gl_Position = position;
}