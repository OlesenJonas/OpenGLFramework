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

layout (location = 3) uniform float materialNormalIntensity = 1.0;
uniform float triplanarSharpness = 0.5;

layout (location = 0) flat in vec2 cornerPoint;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 worldPos;

layout(std430, binding = 3) buffer textureInfoBuffer
{   
    float textureScales[];
};

// UE4's RandFast function
// https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/Private/Random.ush
//  from: https://www.shadertoy.com/view/XlGcRh
float fast(vec2 v)
{
    v = (1./4320.) * v + vec2(0.25,0.);
    float state = fract( dot( v * v, vec2(3571)));
    return fract( state * state * (3571. * 2.));
}

// reorient normal blending for blending material normal maps and terrain normal:
// https://blog.selfshadow.com/publications/blending-in-detail/
vec3 reorientNormalBlend(vec3 t, vec3 u)
{
    t += vec3(0,0,1);
    u *= vec3(-1,-1,1);
    return normalize(t*dot(t, u) - u*t.z);
}

//  KEEP BRANCHING IN MIND! see https://666uille.files.wordpress.com/2017/03/gdc2017_ghostreconwildlands_terrainandtechnologytools-onlinevideos1.pdf

#include "FetchAttributesFromID.glsl"

void main()
{
    float gs = 0.2+0.6*fast(cornerPoint);
    gs = pow(gs,2.2);

    const uint closestMaterialID = texture(materialIDTex, uv).r;
    const float materialIDVis = (closestMaterialID & 0x7F)/6.0;

    vec3 macroNormal = 2.0*texture(macroNormal, uv).xyz-1.0;

    const vec2 scaledUVs = uv*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);

    //need to explicitly calculate derivatives before scaling by textureScale, otherweise pixel neighbours
    //can have discontinuities in their UVs
    const vec3 dPdx = dFdx(worldPos);
    const vec3 dPdy = dFdy(worldPos);

    const MaterialAttributes maFF = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(0,0), worldPos, dPdx, dPdy, macroNormal);
    const MaterialAttributes maFC = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(0,1), worldPos, dPdx, dPdy, macroNormal);
    const MaterialAttributes maCF = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(1,0), worldPos, dPdx, dPdy, macroNormal);
    const MaterialAttributes maCC = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(1,1), worldPos, dPdx, dPdy, macroNormal);
    const MaterialAttributes maF = lerp(maFF, maFC, weights.y);
    const MaterialAttributes maC = lerp(maCF, maCC, weights.y);
    const MaterialAttributes attributes = lerp(maF, maC, weights.x);
    const vec3 diffuse = attributes.diffuseRoughness.rgb;
    const float roughness = attributes.diffuseRoughness.w;
    const float ambientOcclusion = attributes.aoHeight.x;
    vec3 tangentNormal = attributes.normalMetallic.xyz;
    //TBN columns would just be (1,0,0),(0,0,-1),(0,1,0), so no need for matrix mult here
    vec3 worldNormal = vec3(tangentNormal.x, tangentNormal.z, -tangentNormal.y);

    // vec3 color = vec3(0.9);
    // // vec3 color = vec3(materialIDVis);
    vec3 color = diffuse * ambientOcclusion;
    color *= max(dot(worldNormal, normalize(vec3(1.0,1.0,0.0))), 0.0) + 0.1;
    fragmentColor = vec4(color,1.0);
}