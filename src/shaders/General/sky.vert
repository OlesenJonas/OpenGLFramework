#version 430

layout (location = 0) in vec4 position;
layout (location = 2) in vec2 textureCoord;

out vec2 passTextureCoord;

void main(){
    passTextureCoord = textureCoord;
    gl_Position = vec4(position.x, position.y, 1, 1);
}