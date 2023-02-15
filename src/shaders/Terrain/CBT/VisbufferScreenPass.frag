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

#include "../../General/CameraMatrices.glsl"
layout(binding = 1) uniform PassMatricesBuffer
{
    CameraMatrices cameraMatrices;
};

#include "../SettingsStruct.glsl"
layout(binding = 4) uniform terrainSettingsBuffer
{
    TerrainSettings terrainSettings;
};

layout(std430, binding = 3) buffer textureInfoBuffer
{   
    float textureScales[];
};

#include "../../General/rnm.glsl"
#define ONLY_DEFINES
#include "transform.glsl"
#define ONLY_FUNCTIONS
#include "FetchAttributes.glsl"
#include "../../General/lighting.glsl"

vec3 worldPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posWorldSpace = cameraMatrices.invProjView * posClipSpace;
    return(posWorldSpace.xyz / posWorldSpace.w);
}
vec3 viewPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posViewSpace = cameraMatrices.invProj * posClipSpace;
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

    vec2 screenUV = gl_FragCoord.xy/vec2(textureSize(visbufferTex,0));
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
    vec2 flatUV = vec2(1,-1) * flatPosition + 0.5;
    // vec2 duvdx = vec2(1,-1)*dPdx.xz/TERRAIN_SIZE;
    // vec2 duvdy = vec2(1,-1)*dPdy.xz/TERRAIN_SIZE;

    // vec3 macroNormal = 2.0*textureGrad(macroNormal, uv, duvdx, duvdy).xyz-1.0;
    const vec3 macroNormalTS = 2.0*textureLod(macroNormal, flatUV, 0).xyz-1.0;
    vec2 biplanarWeights = pow(abs(macroNormalTS.xy),vec2(terrainSettings.triplanarSharpness));
    biplanarWeights /= biplanarWeights.x + biplanarWeights.y;
    float biplanarWeight = biplanarWeights.x;

    const vec2 flipFactorXY = vec2(-sign(macroNormalTS.y), 1);
    const vec2 flipFactorZY = vec2(-sign(macroNormalTS.x), 1);

    vec2 uvXZ = worldPosNoDisplacement.xz;
    vec2 dPdxXZ = dPdx.xz;
    vec2 dPdyXZ = dPdy.xz;
    
    vec2 uvXY = worldPosNoDisplacement.xy*flipFactorXY;
    vec2 dPdxXY = dPdx.xy*flipFactorXY;
    vec2 dPdyXY = dPdy.xy*flipFactorXY;

    vec2 uvZY = worldPosNoDisplacement.zy*flipFactorZY;
    vec2 dPdxZY = dPdx.zy*flipFactorZY;
    vec2 dPdyZY = dPdy.zy*flipFactorZY;

    const vec2 scaledUVs = flatUV*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);
    const uint materialIDs[4] = uint[4](
        texelFetch(materialIDTex, idsStartTexel+ivec2(0,0), 0).r,
        texelFetch(materialIDTex, idsStartTexel+ivec2(0,1), 0).r,
        texelFetch(materialIDTex, idsStartTexel+ivec2(1,0), 0).r,
        texelFetch(materialIDTex, idsStartTexel+ivec2(1,1), 0).r
    );

    MaterialAttributes matAttributes[4];
    for(int i=0; i<4; i++)
    {
        if((materialIDs[i] & 0x80) > 0)
        {
            matAttributes[i] = biplanarSampleOfMaterialAttributesFromID(
                materialIDs[i] & 0x7F, uvXY, dPdxXY, dPdyXY, flipFactorXY, uvZY, dPdxZY, dPdyZY, flipFactorZY, biplanarWeight, macroNormalTS
            );
        }
        else
        {
            matAttributes[i] = flatSampleOfMaterialAttributesFromID(
                materialIDs[i] & 0x7F, uvXZ, dPdxXZ, dPdyXZ, macroNormalTS
            );
        }
    }

    const MaterialAttributes maF = lerp(matAttributes[0], matAttributes[1], weights.y);
    const MaterialAttributes maC = lerp(matAttributes[2], matAttributes[3], weights.y);
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
	const vec3 viewNormal = normalize(cameraMatrices.View * vec4(worldNormal, 0)).xyz;

	vec3 baseColor = diffuse;
	const vec3 reflect = mix(vec3(0.04f, 0.04f, 0.04f), baseColor.xyz, 0.0f);

	directIllumination(cameraMatrices.View, V, P, viewNormal, worldPos, LightColor.xyz, LightDirection, baseColor, roughness, diff, spec);
	imageBasedLighting(cameraMatrices.View, V, viewNormal, worldNormal, reflect, 0.0f, baseColor, diff, spec, roughness, ambientOcclusion);

    diff = any(isnan(diff)) ? vec3(0.0) : diff;
    spec = any(isnan(spec)) ? vec3(0.0) : spec;
	const vec3 col = diff + spec;
    fragmentColor = vec4(col, 1);
}