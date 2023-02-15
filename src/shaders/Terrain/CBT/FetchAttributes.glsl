#ifndef FETCHFROMID_GLSL
#define FETCHFROMID_GLSL

#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded
    #ifndef ONLY_FUNCTIONS
        layout (binding = 1) uniform sampler2D macroNormal;
        layout (binding = 2) uniform usampler2D materialIDTex;
        layout (binding = 4) uniform sampler2DArray diffuseArray;
        layout (binding = 5) uniform sampler2DArray normalArray;
        layout (binding = 6) uniform sampler2DArray ordArray;
        layout (binding = 7) uniform sampler2D visbufferTex;
        layout (binding = 8) uniform sampler2D posTex;
        layout (binding = 9) uniform sampler2D trueDepthBuffer;
        layout(std430, binding = 3) buffer textureInfoBuffer
        {   
            float textureScales[];
        };
        #include "../SettingsStruct.glsl"
        layout(binding = 4) uniform terrainSettingsBuffer
        {
            TerrainSettings terrainSettings;
        };
    #endif
#endif

#include "../../General/rnm.glsl"
#include "../../General/MaterialAttributes.glsl"
#include "../../General/Blending.glsl"


MaterialAttributes flatSampleOfMaterialAttributesFromID(
    const uint materialID,
    vec2 uv, vec2 duvdx, vec2 duvdy,
    const vec3 macroNormalTS)
{
    const float texScale = textureScales[materialID];
    uv = uv/texScale;
    duvdx = duvdx/texScale;
    duvdy = duvdy/texScale;
    vec3 nrm = 2*textureGrad(normalArray, vec3(uv,materialID), duvdx, duvdy).rgb-1;
    nrm = normalize(mix(vec3(0,0,1),nrm,terrainSettings.materialNormalIntensity));
    nrm = reorientNormalBlend(macroNormalTS, nrm);
    return CreateMaterialAttributes(
        textureGrad(diffuseArray, vec3(uv,materialID), duvdx, duvdy).rgb,
        nrm,
        textureGrad(ordArray, vec3(uv,materialID), duvdx, duvdy).rgb
    ); 
}

MaterialAttributes biplanarSampleOfMaterialAttributesFromID(
    const uint materialID,
    vec2 uvXY, vec2 dPdxXY, vec2 dPdyXY, const vec2 flipFactorXY,
    vec2 uvZY, vec2 dPdxZY, vec2 dPdyZY, const vec2 flipFactorZY,
    const float weight, const vec3 macroNormalTS)
{
    const float texScale = textureScales[materialID];

    uvXY /= texScale;
    uvZY /= texScale;
    
    dPdxXY /= texScale;
    dPdyXY /= texScale;
    dPdxZY /= texScale;
    dPdyZY /= texScale;

    vec3 nrmXY = normalize(
            mix(
                vec3(0,0,1),
                2*textureGrad(normalArray, vec3(uvXY,materialID), dPdxXY, dPdyXY).rgb-1,
                terrainSettings.materialNormalIntensity
            ));
    nrmXY.xy *= flipFactorXY.x;
    nrmXY = reorientNormalBlend(macroNormalTS, nrmXY);
    vec3 nrmZY = normalize(
            mix(
                vec3(0,0,1),
                2*textureGrad(normalArray, vec3(uvZY,materialID), dPdxZY, dPdyZY).rgb-1,
                terrainSettings.materialNormalIntensity
            ));
    nrmZY.xy  = nrmZY.yx;
    nrmZY.x *= flipFactorZY.x;
    nrmZY.y *= -flipFactorZY.x;
    nrmZY = reorientNormalBlend(macroNormalTS, nrmZY);

    return lerp(
        CreateMaterialAttributes(
            textureGrad(diffuseArray, vec3(uvXY,materialID), dPdxXY, dPdyXY).rgb,
            nrmXY,
            textureGrad(ordArray, vec3(uvXY,materialID), dPdxXY, dPdyXY).rgb
        ), 
        CreateMaterialAttributes(
            textureGrad(diffuseArray, vec3(uvZY,materialID), dPdxZY, dPdyZY).rgb,
            nrmZY,
            textureGrad(ordArray, vec3(uvZY,materialID), dPdxZY, dPdyZY).rgb
        ),
        weight);
}

MaterialAttributes getFlatMaterialAttributesFromIndexAndWorldPos(const uint materialID, vec3 worldPos, vec3 dPdx, vec3 dPdy, const vec3 macroNormalTS)
{
    const float texScale = textureScales[materialID];
    worldPos = worldPos/texScale;
    dPdx = dPdx/texScale;
    dPdy = dPdy/texScale;
    vec3 nrm = 2*textureGrad(normalArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb-1;
    nrm = normalize(mix(vec3(0,0,1),nrm,terrainSettings.materialNormalIntensity));
    nrm = reorientNormalBlend(macroNormalTS, nrm);
    return CreateMaterialAttributes(
        textureGrad(diffuseArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb,
        nrm,
        textureGrad(ordArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb
    );
}
MaterialAttributes getBiplanarMaterialAttributesFromIndexAndWorldPos(const uint materialID, vec3 worldPos, vec3 dPdx, vec3 dPdy, const vec3 macroNormalTS)
{
    const float texScale = textureScales[materialID];
    
    const vec2 flipFactorXY = vec2(-sign(macroNormalTS.y), 1);
    const vec2 flipFactorZY = vec2(-sign(macroNormalTS.x), 1);
    
    const vec2 uvXY = worldPos.xy*flipFactorXY/ texScale;
    const vec2 uvZY = worldPos.zy*flipFactorZY/ texScale;
    
    const vec2 dPdxXY = dPdx.xy*flipFactorXY/texScale;
    const vec2 dPdyXY = dPdy.xy*flipFactorXY/texScale;
    const vec2 dPdxZY = dPdx.zy*flipFactorZY/texScale;
    const vec2 dPdyZY = dPdy.zy*flipFactorZY/texScale;

    vec3 nrmXY = normalize(
            mix(
                vec3(0,0,1),
                2*textureGrad(normalArray, vec3(uvXY,materialID), dPdxXY, dPdyXY).rgb-1,
                terrainSettings.materialNormalIntensity
            ));
    vec3 nrmZY = normalize(
            mix(
                vec3(0,0,1),
                2*textureGrad(normalArray, vec3(uvZY,materialID), dPdxZY, dPdyZY).rgb-1,
                terrainSettings.materialNormalIntensity
            ));
    nrmXY.xy *= flipFactorXY.x;
    nrmZY.xy  = nrmZY.yx;
    nrmZY.x *= flipFactorZY.x;
    nrmZY.y *= -flipFactorZY.x;
    nrmXY = reorientNormalBlend(macroNormalTS, nrmXY);
    nrmZY = reorientNormalBlend(macroNormalTS, nrmZY);

    const MaterialAttributes materialAttributesXY = CreateMaterialAttributes(
        textureGrad(diffuseArray, vec3(uvXY,materialID), dPdxXY, dPdyXY).rgb,
        nrmXY,
        textureGrad(ordArray, vec3(uvXY,materialID), dPdxXY, dPdyXY).rgb
    );
    const MaterialAttributes materialAttributesZY = CreateMaterialAttributes(
        textureGrad(diffuseArray, vec3(uvZY,materialID), dPdxZY, dPdyZY).rgb,
        nrmZY,
        textureGrad(ordArray, vec3(uvZY,materialID), dPdxZY, dPdyZY).rgb
    );

    vec2 weights = pow(abs(macroNormalTS.xy),vec2(terrainSettings.triplanarSharpness));
    weights /= weights.x+weights.y;

    return lerp(materialAttributesXY, materialAttributesZY, weights.x);
}
MaterialAttributes getMaterialAttributesFromTexelAndWorldPos(const ivec2 texelPos, const vec3 worldPos, const vec3 dPdx, const vec3 dPdy, const vec3 macroNormalTS)
{
    uint materialID = texelFetch(materialIDTex, texelPos, 0).r;
    bool isTriplanar = (materialID & 0x80) > 0;
    materialID = materialID & 0x7F;
    if(isTriplanar)
        return getBiplanarMaterialAttributesFromIndexAndWorldPos(materialID, worldPos, dPdx, dPdy, macroNormalTS);
    else
        return getFlatMaterialAttributesFromIndexAndWorldPos(materialID, worldPos, dPdx, dPdy, macroNormalTS);
}
MaterialAttributes getFlatMaterialAttributesFromTexelAndWorldPos(const ivec2 texelPos, const vec3 worldPos, const vec3 dPdx, const vec3 dPdy, const vec3 macroNormalTS)
{
    uint materialID = texelFetch(materialIDTex, texelPos, 0).r;
    materialID = materialID & 0x7F;
    return getFlatMaterialAttributesFromIndexAndWorldPos(materialID, worldPos, dPdx, dPdy, macroNormalTS);
}
MaterialAttributes getBiplanarMaterialAttributesFromTexelAndWorldPos(const ivec2 texelPos, const vec3 worldPos, const vec3 dPdx, const vec3 dPdy, const vec3 macroNormalTS)
{
    uint materialID = texelFetch(materialIDTex, texelPos, 0).r;
    materialID = materialID & 0x7F;
    return getBiplanarMaterialAttributesFromIndexAndWorldPos(materialID, worldPos, dPdx, dPdy, macroNormalTS);
}
#endif
