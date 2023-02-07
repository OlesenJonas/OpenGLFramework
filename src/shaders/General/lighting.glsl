const float Pi = 3.14159265358979323846;

uniform layout (binding = 10) samplerCube irradianceMap;
uniform layout (binding = 11) samplerCube environmentMap;
uniform layout (binding = 12) sampler2D brdf;
uniform layout (binding = 13) samplerCube sky;
uniform layout (binding = 14) sampler2DShadow sunShadowmap;

layout (binding = 21) uniform Lightbuffer
{
	mat4 shadowMatrix;
    vec4 LightDirection;
    vec4 LightColor;
	float IndirectLightExposure;
};  

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
float ShadowCoverage(in const vec3 worldPos)
{
	vec4 lightPos = shadowMatrix * vec4(worldPos, 1);
	vec3 projCoords = lightPos.xyz / lightPos.w;
	projCoords = projCoords * 0.5f + 0.5f; 
	const float offset = 1.0/4096.0f;

	// Reference: https://ogldev.org/www/tutorial42/tutorial42.html
    float sum = 0.0f;
    for (int y = -1 ; y <= 1 ; y++) 
	{
        for (int x = -1 ; x <= 1 ; x++) 
		{
            const vec3 uvc = vec3(projCoords.xy + vec2(x * offset, y * offset), projCoords.z - 0.00001f);
            sum += texture(sunShadowmap, uvc).x;
        }
    }
	return (sum / 9.0f);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
void directIllumination(in mat4 view, in vec3 V, in vec3 P, in vec3 N, in vec3 worldPos, in vec3 lightColor, in vec4 lightDir, in vec3 baseColor, in float r, inout vec3 diffuse, inout vec3 specular)
{

	// Shadow
	const float shadowTerm = ShadowCoverage(worldPos);
	if (shadowTerm <= 0)
	{
		return;
	}

    const vec3 L = normalize(view * lightDir).xyz;

	const vec3 H = normalize(L + V);
	const float NdotL = saturate(dot(N, L));
	const float NdotV = saturate(dot(N, V));
	const float NdotH = saturate(dot(N, H));
	const float LdotH = saturate(dot(L, H));
	const float LdotV = saturate(dot(L, V));
	const vec3 c = lightColor * NdotL;
	const vec3 F0 = vec3(0.1,0.1,0.1);
	const float r2 = r * r;

	diffuse += diffuseBRDF(N, L, V, NdotL, NdotV, LdotV, baseColor, r2) * c * shadowTerm;
	specular += specularBRDF(NdotL, NdotV, NdotH, LdotH, r2, F0) * c * shadowTerm;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
void imageBasedLighting(in mat4 view, in vec3 V, in vec3 N, in vec3 wN, in vec3 reflectance, inout vec3 diffuse, inout vec3 specular, in float roughness, in float ao)
{
	const float NdotV = saturate(dot(N, V));

	const vec3 R = reflect(-V, N);

	vec4 viewR = inverse(view) * vec4(R,0);

    const vec3 preFilteredEnvironment = textureLod(environmentMap, viewR.xyz, roughness * 8.0f).xyz * reflectance * IndirectLightExposure;
	const vec2 brdfIntegral	= texture(brdf, vec2(roughness, NdotV)).xy;
	const vec3 specularIB = preFilteredEnvironment * (reflectance * brdfIntegral.x + brdfIntegral.y);
    
	diffuse += reflectance * textureLod(irradianceMap, wN, 0.0f).xyz * IndirectLightExposure * ao;
	specular += specularIB;
}
