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

layout (location = 3) uniform float materialNormalIntensity = 1.0f;

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

#include "../../General/Blending.glsl"

// Uses reorient normal blending for blending material normal maps and terrain normal:
// https://blog.selfshadow.com/publications/blending-in-detail/

// TODO: Triplanar
//  Who decides if a sample needs to be triplanar? Store slope information in materialID texture?
//  KEEP BRANCHING IN MIND! see https://666uille.files.wordpress.com/2017/03/gdc2017_ghostreconwildlands_terrainandtechnologytools-onlinevideos1.pdf
MaterialAttributes getMaterialAttributesFromTexelAndWorldPos(const ivec2 texelPos, vec3 worldPos, vec3 dPdx, vec3 dPdy)
{
    const uint materialID = texelFetch(materialIDTex, texelPos, 0).r;
    const float texScale = textureScales[materialID];
    worldPos = worldPos/texScale;
    dPdx = dPdx/texScale;
    dPdy = dPdy/texScale;
    return CreateMaterialAttributes(
        textureGrad(diffuseArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb,
        2*textureGrad(normalArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb-1,
        textureGrad(ordArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb
    );
}

void main()
{
    float gs = 0.2+0.6*fast(cornerPoint);
    gs = pow(gs,2.2);

    const uint closestMaterialID = texture(materialIDTex, uv).r;
    const float materialIDVis = closestMaterialID/6.0;

    const vec2 scaledUVs = uv*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);

    //need to explicitly calculate derivatives before scaling by textureScale, otherweise pixel neighbours
    //can have discontinuities in their UVs
    const vec3 dPdx = dFdx(worldPos);
    const vec3 dPdy = dFdy(worldPos);

    const MaterialAttributes maFF = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(0,0), worldPos, dPdx, dPdy);
    const MaterialAttributes maFC = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(0,1), worldPos, dPdx, dPdy);
    const MaterialAttributes maCF = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(1,0), worldPos, dPdx, dPdy);
    const MaterialAttributes maCC = getMaterialAttributesFromTexelAndWorldPos(idsStartTexel+ivec2(1,1), worldPos, dPdx, dPdy);
    const MaterialAttributes maF = lerp(maFF, maFC, weights.y);
    const MaterialAttributes maC = lerp(maCF, maCC, weights.y);
    const MaterialAttributes attributes = lerp(maF, maC, weights.x);
    const vec3 diffuse = attributes.diffuseRoughness.rgb;
    const float roughness = attributes.diffuseRoughness.w;
    vec3 materialNormal = attributes.normalMetallic.xyz;
    const float ambientOcclusion = attributes.aoHeight.x;

    vec3 macroNormal = texture(macroNormal, uv).xyz;
    //macro normal is only ever used as a base for reorient normal blending, so transform directly here
    macroNormal = vec3(2,2,2) * macroNormal - vec3(1, 1, 0);

    materialNormal = normalize(
        mix(    //to control the intensity just lerp towards a flat normal
            vec3(0,0,1),
            materialNormal,
            materialNormalIntensity
        )
    );
    //transform for reorient blending
    materialNormal = vec3(-1,-1, 1) * materialNormal;

    const vec3 t = macroNormal;
    const vec3 u = materialNormal; 
    // vec3 blendedTangentNormal = t*dot(t, u)/t.z - u;
    vec3 tangentNormal = normalize(t*dot(t, u) - u*t.z);
    //TBN columns would just be (1,0,0),(0,0,-1),(0,1,0), so no need for matrix mult here
    vec3 worldNormal = vec3(tangentNormal.x, tangentNormal.z, -tangentNormal.y);
    

    // vec3 color = vec3(0.5);
    // vec3 color = vec3(materialIDVis);
    vec3 color = diffuse * ambientOcclusion;
    color *= max(dot(worldNormal, normalize(vec3(1.0,1.0,0.0))), 0.0) + 0.1;

    fragmentColor = vec4(color,1.0);
}