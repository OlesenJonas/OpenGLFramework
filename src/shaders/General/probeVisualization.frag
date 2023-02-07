#version 430

uniform layout (binding = 1) sampler2D albedoMap;
uniform layout (binding = 2) sampler2D normalMap;
uniform layout (binding = 3) sampler2D attributesMap;


layout (location = 1) uniform mat4 viewMatrix;

layout (binding = 22) uniform Materialbuffer
{
    vec4 MaterialColor;
	float NormalIntensity;
};  

out vec4 fragmentColor;

in VSOutput
{
	vec4 worldPos;
    vec4 ViewPos;
    vec2 TexCoord;
    vec3 Normal;
    vec3 WorldNormal;
	mat3 TangentFrame;
} Input;

#include "lighting.glsl"


//--------------------------------------------------------------------------------------------------------------------------------------------
void main()
{
	const vec3 P = Input.ViewPos.xyz;
	const vec3 V = normalize(-P);
	vec3 wNormal = Input.TangentFrame * vec3(0,0,1);
	

    fragmentColor = vec4(Input.WorldNormal.xyz, 1);

	fragmentColor.xyz = textureLod(environmentMap, Input.WorldNormal.xyz, 0).xyz;
	fragmentColor.a = 1.0f;
}