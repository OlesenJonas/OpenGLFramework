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

vec3 reconstructNormalY(const float x, const float z)
{
    return vec3(
        x,
        sqrt(1-x*x-z*z),
        z
    );
}

vec2 getUVFromProjectionAxis(const vec3 projectionAxis, const vec3 worldPos)
{
    const vec3 tempRight = cross(vec3(0,1,0),projectionAxis);
    const vec3 tempUp = cross(projectionAxis,tempRight);
    return vec2(dot(worldPos,tempRight), dot(worldPos, tempUp));
}

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

    const uvec3 sampleInfoFF = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(0,0)).rgb;
    const uvec3 sampleInfoFC = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(0,1)).rgb;
    const uvec3 sampleInfoCF = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(1,0)).rgb;
    const uvec3 sampleInfoCC = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(1,1)).rgb;

    const uint materialidFF = sampleInfoFF.r;
    const uint materialidFC = sampleInfoFC.r;
    const uint materialidCF = sampleInfoCF.r;
    const uint materialidCC = sampleInfoCC.r;

    const vec2 sampleDirxzFF = 2.0*(sampleInfoFF.gb/255.0)-1.0;
    const vec3 sampleNrmFF = reconstructNormalY(sampleDirxzFF.x, sampleDirxzFF.y);
    const vec2 sampleDirxzFC = 2.0*(sampleInfoFC.gb/255.0)-1.0;
    const vec3 sampleNrmFC = reconstructNormalY(sampleDirxzFC.x, sampleDirxzFC.y);
    const vec2 sampleDirxzCF = 2.0*(sampleInfoCF.gb/255.0)-1.0;
    const vec3 sampleNrmCF = reconstructNormalY(sampleDirxzCF.x, sampleDirxzCF.y);
    const vec2 sampleDirxzCC = 2.0*(sampleInfoCC.gb/255.0)-1.0;
    const vec3 sampleNrmCC = reconstructNormalY(sampleDirxzCC.x, sampleDirxzCC.y);

    // const vec3 samplePosFF = vec3(worldPos.xz/textureScales[materialidFF], materialidFF);
    // const vec3 samplePosFC = vec3(worldPos.xz/textureScales[materialidFC], materialidFC);
    // const vec3 samplePosCF = vec3(worldPos.xz/textureScales[materialidCF], materialidCF);
    // const vec3 samplePosCC = vec3(worldPos.xz/textureScales[materialidCC], materialidCC);
    const vec3 samplePosFF = vec3(getUVFromProjectionAxis(sampleNrmFF,worldPos)/textureScales[materialidFF], materialidFF);
    const vec3 samplePosFC = vec3(getUVFromProjectionAxis(sampleNrmFC,worldPos)/textureScales[materialidFC], materialidFC);
    const vec3 samplePosCF = vec3(getUVFromProjectionAxis(sampleNrmCF,worldPos)/textureScales[materialidCF], materialidCF);
    const vec3 samplePosCC = vec3(getUVFromProjectionAxis(sampleNrmCC,worldPos)/textureScales[materialidCC], materialidCC);

    // TODO: Triplanar
    //  Who decides if a sample needs to be triplanar? Store slope information in materialID texture?
    //  KEEP BRANCHING IN MIND! see https://666uille.files.wordpress.com/2017/03/gdc2017_ghostreconwildlands_terrainandtechnologytools-onlinevideos1.pdf
    const vec3 diffuseFF = texture(diffuseArray, samplePosFF).rgb;
    const vec3 diffuseFC = texture(diffuseArray, samplePosFC).rgb;
    const vec3 diffuseCF = texture(diffuseArray, samplePosCF).rgb;
    const vec3 diffuseCC = texture(diffuseArray, samplePosCC).rgb;
    const vec3 normalFF = 2*texture(normalArray, samplePosFF).rgb-1;
    const vec3 normalFC = 2*texture(normalArray, samplePosFC).rgb-1;
    const vec3 normalCF = 2*texture(normalArray, samplePosCF).rgb-1;
    const vec3 normalCC = 2*texture(normalArray, samplePosCC).rgb-1;
    const vec3 ordFF = texture(ordArray, samplePosFF).rgb;
    const vec3 ordFC = texture(ordArray, samplePosFC).rgb;
    const vec3 ordCF = texture(ordArray, samplePosCF).rgb;
    const vec3 ordCC = texture(ordArray, samplePosCC).rgb;
    //This would be more readable and also easier to abstract further imo, but at least atm its more expensive
    // const MaterialAttributes maFF = CreateMaterialAttributes(diffuseFF, normalFF, ordFF);
    // const MaterialAttributes maFC = CreateMaterialAttributes(diffuseFC, normalFC, ordFC);
    // const MaterialAttributes maCF = CreateMaterialAttributes(diffuseCF, normalCF, ordCF);
    // const MaterialAttributes maCC = CreateMaterialAttributes(diffuseCC, normalCC, ordCC);
    // const MaterialAttributes maF = lerp(maFF, maFC, weights.y);
    // const MaterialAttributes maC = lerp(maCF, maCC, weights.y);
    // const MaterialAttributes attributes = lerp(maF, maC, weights.x);
    // const vec3 diffuse = attributes.diffuseRoughness.rgb;
    // const float roughness = attributes.diffuseRoughness.w;
    // vec3 materialNormal = attributes.normalMetallic.xyz;
    // const float ambientOcclusion = attributes.aoHeight.x;

    const vec3 ordF = mix(ordFF, ordFC, weights.y);
    const vec3 ordC = mix(ordCF, ordCC, weights.y);
    const vec3 ord = mix(ordF, ordC, weights.x);
    const float roughness = ord.g;
    const float ambientOcclusion = ord.r;

    const vec3 diffuseF = mix(diffuseFF, diffuseFC, weights.y);
    const vec3 diffuseC = mix(diffuseCF, diffuseCC, weights.y);
    vec3 diffuse = mix(diffuseF, diffuseC, weights.x);

    const vec3 normalF = mix(normalFF, normalFC, weights.y);
    const vec3 normalC = mix(normalCF, normalCC, weights.y);
    vec3 materialNormal = mix(normalF, normalC, weights.x);




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