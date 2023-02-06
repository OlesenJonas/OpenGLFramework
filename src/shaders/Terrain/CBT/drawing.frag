#version 430

out vec4 fragmentColor;

layout (location = 0) uniform mat4 projectionViewMatrix;

layout (binding = 1) uniform sampler2D macroNormal;
layout (binding = 2) uniform usampler2D materialIDTex;

layout (binding = 4) uniform sampler2DArray diffuseArray;
layout (binding = 5) uniform sampler2DArray normalArray;
layout (binding = 6) uniform sampler2DArray ordArray;

layout (location = 3) uniform float materialNormalIntensity = 1.0f;
layout (location = 4) uniform mat4 viewMatrix;

layout (location = 0) flat in vec2 cornerPoint;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 worldPos;
layout (location = 3) in vec3 viewPos;


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

#include "../../General/lighting.glsl"

// Uses reorient normal blending for blending material normal maps and terrain normal:
// https://blog.selfshadow.com/publications/blending-in-detail/

void main()
{
    float gs = 0.2+0.6*fast(cornerPoint);
    gs = pow(gs,2.2);

    const uint closestMaterialID = texture(materialIDTex, uv).r;
    const float materialIDVis = closestMaterialID/6.0;

    const vec2 scaledUVs = uv*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);

    // TODO: replace with a single textureGather call?
    const uint materialidFF = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(0,0)).r;
    const uint materialidFC = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(0,1)).r;
    const uint materialidCF = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(1,0)).r;
    const uint materialidCC = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(1,1)).r;

    const vec3 samplePosFF = vec3(worldPos.xz/textureScales[materialidFF], materialidFF);
    const vec3 samplePosFC = vec3(worldPos.xz/textureScales[materialidFC], materialidFC);
    const vec3 samplePosCF = vec3(worldPos.xz/textureScales[materialidCF], materialidCF);
    const vec3 samplePosCC = vec3(worldPos.xz/textureScales[materialidCC], materialidCC);
    // TODO: Triplanar
    //  Who decides if a sample needs to be triplanar? Store slope information in materialID texture?
    //  KEEP BRANCHING IN MIND! see https://666uille.files.wordpress.com/2017/03/gdc2017_ghostreconwildlands_terrainandtechnologytools-onlinevideos1.pdf
    const vec3 diffuseFF = texture(diffuseArray, samplePosFF).rgb;
    const vec3 diffuseFC = texture(diffuseArray, samplePosFC).rgb;
    const vec3 diffuseCF = texture(diffuseArray, samplePosCF).rgb;
    const vec3 diffuseCC = texture(diffuseArray, samplePosCC).rgb;
    const vec3 diffuseF = mix(diffuseFF, diffuseFC, weights.y);
    const vec3 diffuseC = mix(diffuseCF, diffuseCC, weights.y);
    vec3 diffuse = mix(diffuseF, diffuseC, weights.x);

    const vec3 normalFF = 2*texture(normalArray, samplePosFF).rgb-1;
    const vec3 normalFC = 2*texture(normalArray, samplePosFC).rgb-1;
    const vec3 normalCF = 2*texture(normalArray, samplePosCF).rgb-1;
    const vec3 normalCC = 2*texture(normalArray, samplePosCC).rgb-1;
    const vec3 normalF = mix(normalFF, normalFC, weights.y);
    const vec3 normalC = mix(normalCF, normalCC, weights.y);
    vec3 materialNormal = normalize(mix(normalF, normalC, weights.x));

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
    
    const vec3 ordFF = texture(ordArray, samplePosFF).rgb;
    const vec3 ordFC = texture(ordArray, samplePosFC).rgb;
    const vec3 ordCF = texture(ordArray, samplePosCF).rgb;
    const vec3 ordCC = texture(ordArray, samplePosCC).rgb;
    const vec3 ordF = mix(ordFF, ordFC, weights.y);
    const vec3 ordC = mix(ordCF, ordCC, weights.y);
    const vec3 ord = mix(ordF, ordC, weights.x);
    
    const float ambientOcclusion = ord.r;
    const float roughness = ord.g;

    // vec3 color = vec3(0.5);
    // vec3 color = vec3(materialIDVis);
    //vec3 color = diffuse;
    //color *= max(dot(worldNormal, normalize(vec3(1.0,1.0,0.0))), 0.0) + 0.1;
    //fragmentColor = vec4(color,1.0);

	// Lighting

	vec3 diff = vec3(0,0,0);
	vec3 spec = vec3(0,0,0);

	const vec3 P = viewPos.xyz;
	const vec3 V = normalize(-P);
	const vec3 viewNormal = normalize(viewMatrix * vec4(worldNormal, 0)).xyz;
	directIllumination(viewMatrix, V, P, viewNormal, LightColor.xyz, LightDirection, diffuse, roughness, diff, spec);
	imageBasedLighting(viewMatrix, V, viewNormal, materialNormal, 1.0f - roughness, diff, spec, roughness);

	const vec3 col = diff + spec;
    fragmentColor = vec4(col, 1);
}