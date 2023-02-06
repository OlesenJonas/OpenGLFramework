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
    vec4 ViewPos;
    vec2 TexCoord;
    vec3 Normal;
	mat3 TangentFrame;
} Input;

#include "lighting.glsl"


//--------------------------------------------------------------------------------------------------------------------------------------------
void main()
{
	const vec3 P = Input.ViewPos.xyz;
	const vec3 V = normalize(-P);

	vec3 diffuse = vec3(0,0,0);
	vec3 specular = vec3(0,0,0);
	
	vec3 normal = texture(normalMap, Input.TexCoord).xyz;
	//normal = pow(normal.xyz, vec3(1.0/2.2));
	//normal.y = 1 - normal.y;
	//normal = vec3(0.5, 0.5, 1.0);
    normal = normalize(mix(vec3(0,0,1), normal * 2.0 - 1.0, NormalIntensity));  
    vec3 worldNormal = Input.TangentFrame * normal;
	normal = (viewMatrix * vec4(worldNormal, 0)).xyz;
	//normal = Input.Normal;
	vec4 attributes = texture(attributesMap, Input.TexCoord);
	
	//TEST
	//attributes.x = 0.8f;

	const float reflectance = 1 - attributes.x;

	const vec3 baseColor = texture(albedoMap, Input.TexCoord).xyz * MaterialColor.xyz;

	directIllumination(viewMatrix, V, P, normal, LightColor.xyz, LightDirection, baseColor, attributes.x, diffuse, specular);
	imageBasedLighting(viewMatrix, V, normal, worldNormal, reflectance, diffuse, specular, attributes.x);

	const vec3 color = diffuse + specular;
    fragmentColor = vec4(color.xyz, 1);
}