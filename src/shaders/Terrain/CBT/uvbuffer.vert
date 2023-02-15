#version 450

#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    //for OpenGL they will be replaced when shader is loaded
    #extension GL_GOOGLE_include_directive : require
#endif

layout(std430, binding = 0) readonly restrict buffer cbtSSBO
{
    uint heap[];
};

#define HEAP_ALREADY_DEFINED
#define HEAP_READ_ONLY
#include "HighLevel.glsl"
#include "Misc.glsl"

layout (location = 0) in vec2 xzPosition;

layout (binding = 0) uniform sampler2D displacementTex;
layout (binding = 1) uniform sampler2D macroNormal;
layout (binding = 2) uniform usampler2D materialIDTex;
layout (binding = 3) uniform sampler2DArray heightArray;

layout (location = 0) out vec3 worldPosNoDisplacement;
// layout (location = 1) out vec4 projPosNoDisplacement;

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

#define DISPLACEMENT_ALREADY_DEFINED
#include "transform.glsl"
#define ONLY_FUNCTIONS
#include "FetchHeight.glsl"

void main()
{
    const Node leafNode = leafIndexToNode(gl_InstanceID);

    //dont render "outer" triangles
    if(!isNodeInCenterSquare(leafNode))
    {
        gl_Position = vec4(0,0,0,0);
        return;
    }

    vec2[3] currentCorners = cornersFromNode(leafNode);

    vec2 flatPosition =                     currentCorners[1] + 
          xzPosition.x * (currentCorners[2]-currentCorners[1]) + 
          xzPosition.y * (currentCorners[0]-currentCorners[1]);

    vec2 flatUV = vec2(1,-1) * flatPosition + 0.5;

    vec4 worldPosition = transformFlatPointToWorldSpace(flatPosition);
    //pass the *non-displaced* position for triplanar projection in fragment shader
    worldPosNoDisplacement = worldPosition.xyz;
    // projPosNoDisplacement = projectionViewMatrix * worldPosition;

    const vec3 macroNormalTS = 2.0*textureLod(macroNormal, flatUV, 0).xyz-1.0;
    vec2 biplanarWeights = pow(abs(macroNormalTS.xy),vec2(terrainSettings.triplanarSharpness));
    biplanarWeights /= biplanarWeights.x + biplanarWeights.y;
    float biplanarWeight = biplanarWeights.x;

    const vec2 flipFactorXY = vec2(-sign(macroNormalTS.y), 1);
    const vec2 flipFactorZY = vec2(-sign(macroNormalTS.x), 1);

    vec2 uvXZ = worldPosNoDisplacement.xz;
    vec2 uvXY = worldPosNoDisplacement.xy*flipFactorXY;
    vec2 uvZY = worldPosNoDisplacement.zy*flipFactorZY;

    const vec2 scaledUVs = flatUV*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);

    const uint materialIDs[4] = uint[4](
        texelFetch(materialIDTex, idsStartTexel+ivec2(0,0), 0).r,
        texelFetch(materialIDTex, idsStartTexel+ivec2(0,1), 0).r,
        texelFetch(materialIDTex, idsStartTexel+ivec2(1,0), 0).r,
        texelFetch(materialIDTex, idsStartTexel+ivec2(1,1), 0).r
    );

    float height;
    //Check if vertex requires just a single material lookup
    if(materialIDs[0] == materialIDs[1] && materialIDs[1] == materialIDs[2] && materialIDs[2] == materialIDs[3])
    {
        if((materialIDs[0] & 128u) == 128u)
        {
            height = biplanarSampleOfHeightFromID(materialIDs[0] & 0x7F, uvXY, uvZY, biplanarWeight);
        }
        else
        {
            height = flatSampleOfHeightFromID(materialIDs[0] & 0x7F, uvXZ);
        }
    }
    else
    {
        float heights[4];
        #pragma optionNV (unroll all)
        for(int i=0; i<4; i++)
        {
            if((materialIDs[i] & 128u) > 0)
            {
                heights[i] = biplanarSampleOfHeightFromID(materialIDs[i] & 0x7F, uvXY, uvZY, biplanarWeight);
            }
            else
            {
                heights[i] = flatSampleOfHeightFromID(materialIDs[i] & 0x7F, uvXZ);
            }
        }
        const float heightF = mix(heights[0], heights[1], weights.y);
        const float heightC = mix(heights[2], heights[3], weights.y);
        height = mix(heightF, heightC, weights.x);
    }

    const float displacement = (height - 0.5) * terrainSettings.materialDisplacementIntensity;

    vec3 worldNormal = vec3(macroNormalTS.x, macroNormalTS.z, -macroNormalTS.y);
    worldPosition += vec4(worldNormal*displacement, 0.0);

    gl_Position = cameraMatrices.ProjView * worldPosition;
}