#version 430

layout (location = 3) uniform mat4 skyProjection;

out vec4 fragmentColor;

in vec2 passTextureCoord;

uniform layout (binding = 10) samplerCube irradianceMap;
uniform layout (binding = 11) samplerCube environmentMap;
uniform layout (binding = 12) sampler2D brdf;
uniform layout (binding = 13) samplerCube sky;

void main() {
    const vec2 coord = (passTextureCoord + vec2(-0.5f, -0.5f)) * vec2(2.0f, 2.0f);
	const vec3 dir = normalize((skyProjection * vec4(coord, 0, 1)).xyz);
    fragmentColor = texture(sky, dir);
}