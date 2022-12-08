#version 430

in vec2 passTexCoord;

uniform layout (binding = 0) sampler2D tex;

out vec4 fragmentColor;

void main() {
    fragmentColor = texture(tex, passTexCoord);
}