#version 430

const float Pi = 3.14159265358979323846;

uniform layout (binding = 1) sampler2D albedoMap;
uniform layout (binding = 2) sampler2D normalMap;
uniform layout (binding = 3) sampler2D attributesMap;
uniform layout (binding = 10) samplerCube irradianceMap;
uniform layout (binding = 11) samplerCube environmentMap;
uniform layout (binding = 12) sampler2D brdf;

layout (location = 1) uniform mat4 viewMatrix;

layout (binding = 21) uniform Lightbuffer
{
    vec4 LightDirection;
    vec4 LightColor;
};  

layout (binding = 22) uniform Materialbuffer
{
    vec4 MaterialColor;
};  

out vec4 fragmentColor;

in VSOutput
{
    vec4 ViewPos;
    vec2 TexCoord;
    vec3 Normal;
	mat3 TangentFrame;
} Input;


//--------------------------------------------------------------------------------------------------------------------------------------------
float saturate(float v)
{
	return clamp(v, 0, 1);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
float specularG1(float NdotV, float k)
{
	return NdotV / (NdotV * (1 - k) + k);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
float specularG(float L, float V, float alpha)
{
	const float k  = alpha / 2;
	const float gl = specularG1(L, k);
	const float gv = specularG1(V, k);

	return gl * gv;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
vec3 specularF(float theta, vec3 F0)
{
	return F0 + (1 - F0) * pow(1 - theta, 5);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
float specularD(float NdotH, float alpha)
{
	const float alpha2 = alpha * alpha;
	const float denom  = NdotH * NdotH * (alpha2 - 1) + 1;
	return alpha2 / max(0.00000001f, Pi * denom * denom);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
vec3 specularBRDF(float NdotL, float NdotV, float NdotH, float LdotH, float alpha, vec3 F0)
{
	const float D = specularD(NdotH, alpha);
	const float G = specularG(NdotL, NdotV, alpha);
	const vec3  F = specularF(LdotH, F0);
	return F * (D * G);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
vec3 diffuseBRDF(vec3 N, vec3 L, vec3 V, float NdotL, float NdotV, float LdotV, vec3 baseColor, float r2)
{
    const float A = 1.0 - 0.5 * (r2 / (r2+0.57));
    const float B = 0.45 * (r2 / (r2 + 0.09));
    const float G = max(0, dot(V - N * NdotV, N - N * NdotL));
	return baseColor * NdotL * (A + B * G * sqrt((1.0 - NdotV * NdotV) * (1.0 - NdotL * NdotL)) / max(NdotL, NdotV));
}

//--------------------------------------------------------------------------------------------------------------------------------------------
void directIllumination(in vec3 V, in vec3 P, in vec3 N, in vec2 uv, inout vec3 diffuse, inout vec3 specular)
{
	const vec3 lightColor = LightColor.xyz;
    const vec3 L = normalize(viewMatrix * LightDirection).xyz;

	const vec3 H = normalize(L + V);
	const float NdotL = saturate(dot(N, L));
	const float NdotV = saturate(dot(N, V));
	const float NdotH = saturate(dot(N, H));
	const float LdotH = saturate(dot(L, H));
	const float LdotV = saturate(dot(L, V));
	const vec3 c = lightColor * NdotL;
	const vec3 baseColor = texture(albedoMap, uv).xyz * vec3(1.0, 0.75, 0.45);
	const vec3 F0 = vec3(0.1,0.1,0.1);
	const float r = texture(attributesMap, uv).x;
	const float r2 = r * r;

	diffuse = diffuseBRDF(N, L, V, NdotL, NdotV, LdotV, baseColor, r2) * c;
	specular = specularBRDF(NdotL, NdotV, NdotH, LdotH, r2, F0) * c;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
void imageBasedLighting(in vec3 V, in vec3 N, in float reflectance, inout vec3 specular, in float roughness)
{
	const float  NdotV	= saturate(dot(N, V));

	const vec3 R = reflect(-V, N);

	vec4 viewR = inverse(viewMatrix) * vec4(R,0);

    const vec3 preFilteredEnvironment = textureLod(environmentMap, viewR.xyz, roughness * 8.0f).xyz * reflectance * 0.5f;
	const vec2 brdfIntegral	= texture(brdf, vec2(roughness, NdotV)).xy;
	const vec3 specularIB = preFilteredEnvironment * (reflectance * brdfIntegral.x + brdfIntegral.y);
    
	specular += specularIB;
}

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
    normal = normalize(normal * 2.0 - 1.0);  
    normal = Input.TangentFrame * normal;
	normal = (viewMatrix * vec4(normal, 0)).xyz;
	//normal = Input.Normal;
	vec4 attributes = texture(attributesMap, Input.TexCoord);
	
	//TEST
	//attributes.x = 0.0f;

	const float reflectance = 1 - attributes.x;

	directIllumination(V, P, normal, Input.TexCoord, diffuse, specular);
	imageBasedLighting(V, normal, reflectance, specular, attributes.x);

	const vec3 color = diffuse + specular;
    fragmentColor = vec4(color.xyz, 1);
}