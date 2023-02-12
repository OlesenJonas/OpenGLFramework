#version 430

#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

out vec4 fragmentColor;

layout (binding = 1) uniform sampler2D macroNormal;
layout (binding = 2) uniform usampler2D materialIDTex;
layout (binding = 4) uniform sampler2DArray diffuseArray;
layout (binding = 5) uniform sampler2DArray normalArray;
layout (binding = 6) uniform sampler2DArray ordArray;
layout (binding = 7) uniform sampler2D visbufferTex;
layout (binding = 8) uniform sampler2D posTex;
layout (binding = 9) uniform sampler2D trueDepthBuffer;

layout (location = 0) uniform mat4 projectionViewMatrix;
layout (location = 4) uniform mat4 viewMatrix;
layout (location = 8) uniform mat4 invProjView;
layout (location = 9) uniform mat4 invProj;

#include "../SettingsStruct.glsl"
layout(binding = 4) uniform terrainSettingsBuffer
{
    TerrainSettings terrainSettings;
};

layout(std430, binding = 3) buffer textureInfoBuffer
{   
    float textureScales[];
};

// Reorient normal blending for blending material normal maps and terrain normal:
// https://blog.selfshadow.com/publications/blending-in-detail/
vec3 reorientNormalBlend(vec3 t, vec3 u)
{
    t += vec3(0,0,1);
    u *= vec3(-1,-1,1);
    return normalize(t*dot(t, u) - u*t.z);
}

#define ONLY_DEFINES
#include "transform.glsl"
#include "FetchAttributesFromID.glsl"
#include "../../General/lighting.glsl"

vec3 worldPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posWorldSpace = invProjView * posClipSpace;
    return(posWorldSpace.xyz / posWorldSpace.w);
}
vec3 viewPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posViewSpace = invProj * posClipSpace;
    return(posViewSpace.xyz / posViewSpace.w);
}

void main()
{
    vec4 visBuffer = texelFetch(visbufferTex, ivec2(gl_FragCoord.xy), 0);
    // vec2 uv = texelFetch(flatUVTex, ivec2(gl_FragCoord.xy), 0).rg;

    if(visBuffer.w == 1)
    {
        discard;
    }

    //TODO: could store linear depth and then use frustum corners for cheaper world space conversion!
    vec2 screenUV = gl_FragCoord.xy/vec2(textureSize(visbufferTex,0));
    float nonDisplacedDepth = visBuffer.a;
    // vec3 worldPosNoDisplacement = worldPositionFromDepth(screenUV, nonDisplacedDepth);
    vec3 worldPosNoDisplacement = texelFetch(posTex, ivec2(gl_FragCoord.xy), 0).rgb;

    float trueDepth = texelFetch(trueDepthBuffer, ivec2(gl_FragCoord.xy), 0).r;
    vec3 viewPos = viewPositionFromDepth(screenUV, trueDepth);
    vec3 worldPos = worldPositionFromDepth(screenUV, trueDepth);

    vec2 dX = unpackHalf2x16(floatBitsToUint(visBuffer.r));
    vec2 dY = unpackHalf2x16(floatBitsToUint(visBuffer.g));
    vec2 dZ = unpackHalf2x16(floatBitsToUint(visBuffer.b));
    vec3 dPdx = vec3(dX.x, dY.x, dZ.x);
    vec3 dPdy = vec3(dX.y, dY.y, dZ.y);

    vec2 flatPosition = worldPosNoDisplacement.xz/TERRAIN_SIZE;
    vec2 uv = vec2(1,-1) * flatPosition + 0.5;
    vec2 duvdx = vec2(1,-1)*dPdx.xz/TERRAIN_SIZE;
    vec2 duvdy = vec2(1,-1)*dPdy.xz/TERRAIN_SIZE;

    const uint closestMaterialID = textureLod(materialIDTex, uv, 0).r;
    const float materialIDVis = (closestMaterialID & 0x7F)/6.0;

    // vec3 macroNormal = 2.0*textureGrad(macroNormal, uv, duvdx, duvdy).xyz-1.0;
    vec3 macroNormal = 2.0*textureLod(macroNormal, uv, 0).xyz-1.0;

    const vec2 scaledUVs = uv*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);

    const MaterialAttributes maFF = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(0,0), worldPosNoDisplacement, dPdx, dPdy, macroNormal);
    const MaterialAttributes maFC = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(0,1), worldPosNoDisplacement, dPdx, dPdy, macroNormal);
    const MaterialAttributes maCF = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(1,0), worldPosNoDisplacement, dPdx, dPdy, macroNormal);
    const MaterialAttributes maCC = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(1,1), worldPosNoDisplacement, dPdx, dPdy, macroNormal);
    const MaterialAttributes maF = lerp(maFF, maFC, weights.y);
    const MaterialAttributes maC = lerp(maCF, maCC, weights.y);
    const MaterialAttributes attributes = lerp(maF, maC, weights.x);
    const vec3 diffuse = attributes.diffuseRoughness.rgb;
    const float roughness = attributes.diffuseRoughness.w;
    const float ambientOcclusion = attributes.aoHeight.x;
    vec3 tangentNormal = attributes.normalMetallic.xyz;
    //TBN columns would just be (1,0,0),(0,0,-1),(0,1,0), so no need for matrix mult here
    vec3 worldNormal = vec3(tangentNormal.x, tangentNormal.z, -tangentNormal.y);

    // Lighting

	vec3 diff = vec3(0,0,0);
	vec3 spec = vec3(0,0,0);

	const vec3 P = viewPos.xyz;
	const vec3 V = normalize(-P);
	const vec3 viewNormal = normalize(viewMatrix * vec4(worldNormal, 0)).xyz;

	vec3 baseColor = diffuse;
	const vec3 reflect = mix(vec3(0.04f, 0.04f, 0.04f), baseColor.xyz, 0.0f);

	directIllumination(viewMatrix, V, P, viewNormal, worldPos, LightColor.xyz, LightDirection, baseColor, roughness, diff, spec);
	imageBasedLighting(viewMatrix, V, viewNormal, worldNormal, reflect, 0.0f, baseColor, diff, spec, roughness, ambientOcclusion);

    diff = any(isnan(diff)) ? vec3(0.0) : diff;
    spec = any(isnan(spec)) ? vec3(0.0) : spec;
	const vec3 col = diff + spec;
    fragmentColor = vec4(col, 1);
}