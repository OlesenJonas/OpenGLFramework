#ifndef FETCHFROMID_GLSL
#define FETCHFROMID_GLSL

#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded
#endif

#include "../../General/MaterialAttributes.glsl"
#include "../../General/Blending.glsl"

MaterialAttributes getFlatMaterialAttributesFromTexelAndWorldPos(const uint materialID, const ivec2 texelPos, vec3 worldPos, vec3 dPdx, vec3 dPdy, const vec3 macroNormalTS)
{
    const float texScale = textureScales[materialID];
    worldPos = worldPos/texScale;
    dPdx = dPdx/texScale;
    dPdy = dPdy/texScale;
    vec3 nrm = 2*textureGrad(normalArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb-1;
    nrm = normalize(mix(vec3(0,0,1),nrm,materialNormalIntensity));
    nrm = reorientNormalBlend(macroNormalTS, nrm);
    return CreateMaterialAttributes(
        textureGrad(diffuseArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb,
        nrm,
        textureGrad(ordArray, vec3(worldPos.xz,materialID), dPdx.xz, dPdy.xz).rgb
    );
}
MaterialAttributes getBiplanarMaterialAttributesFromTexelAndWorldPos(const uint materialID, const ivec2 texelPos, vec3 worldPos, vec3 dPdx, vec3 dPdy, const vec3 macroNormalTS)
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
                materialNormalIntensity
            ));
    vec3 nrmZY = normalize(
            mix(
                vec3(0,0,1),
                2*textureGrad(normalArray, vec3(uvZY,materialID), dPdxZY, dPdyZY).rgb-1,
                materialNormalIntensity
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

    vec2 weights = pow(abs(macroNormalTS.xy),vec2(triplanarSharpness));
    weights /= weights.x+weights.y;

    return lerp(materialAttributesXY, materialAttributesZY, weights.x);
}
MaterialAttributes getMaterialAttributesFromTexelAndWorldPos(const ivec2 texelPos, const vec3 worldPos, const vec3 dPdx, const vec3 dPdy, const vec3 macroNormalTS)
{
    uint materialID = texelFetch(materialIDTex, texelPos, 0).r;
    bool isTriplanar = (materialID & 0x80) > 0;
    materialID = materialID & 0x7F;
    if(isTriplanar)
        return getBiplanarMaterialAttributesFromTexelAndWorldPos(materialID, texelPos, worldPos, dPdx, dPdy, macroNormalTS);
    else
        return getFlatMaterialAttributesFromTexelAndWorldPos(materialID, texelPos, worldPos, dPdx, dPdy, macroNormalTS);
}

#endif