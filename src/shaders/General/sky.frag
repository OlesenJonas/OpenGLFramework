#version 430

layout (location = 3) uniform mat4 skyProjection;

out vec4 fragmentColor;

in vec2 passTextureCoord;

uniform samplerCube sky;

void main() {
    const vec2 coord = vec2(2, 1) * passTextureCoord + vec2(-1, 0.5);
	const vec3 dir = normalize((skyProjection * vec4(coord, 0, 1)).xyz);

    fragmentColor = texture(sky, dir);
}