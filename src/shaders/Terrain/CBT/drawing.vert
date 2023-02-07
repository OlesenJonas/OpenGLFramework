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

layout (location = 0) uniform mat4 projectionViewMatrix;
layout (location = 1) uniform float materialDisplacementIntensity = 0.0;
layout (location = 2) uniform int materialDisplacementLodOffset = 2;
layout (location = 4) uniform mat4 viewMatrix;
uniform float triplanarSharpness = 0.5;

layout (location = 0) flat out vec2 cornerPoint;
layout (location = 1) out vec2 uv;
layout (location = 2) out vec3 worldPosNoDisplacement;
layout (location = 3) out vec3 worldPos;
layout (location = 4) out vec3 viewPos;

layout(std430, binding = 3) buffer textureInfoBuffer
{   
    float textureScales[];
};

#define DISPLACEMENT_ALREADY_DEFINED
#include "transform.glsl"
#include "FetchHeightFromID.glsl"

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

    uv = vec2(1,-1) * flatPosition + 0.5;

    vec4 worldPosition = transformFlatPointToWorldSpace(flatPosition);
    //pass the *non-displaced* position for triplanar projection in fragment shader
    worldPosNoDisplacement = worldPosition.xyz;

    vec3 tangentNormal = 2.0*textureLod(macroNormal,uv,0).xyz-1.0;
    vec3 worldNormal = vec3(tangentNormal.x, tangentNormal.z, -tangentNormal.y);

    const vec2 scaledUVs = uv*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);

    const float heightFF = getHeightFromTexelAndWorldPos(idsStartTexel+ivec2(0,0), worldPosition.xyz, tangentNormal);
    const float heightFC = getHeightFromTexelAndWorldPos(idsStartTexel+ivec2(0,1), worldPosition.xyz, tangentNormal);
    const float heightCF = getHeightFromTexelAndWorldPos(idsStartTexel+ivec2(1,0), worldPosition.xyz, tangentNormal);
    const float heightCC = getHeightFromTexelAndWorldPos(idsStartTexel+ivec2(1,1), worldPosition.xyz, tangentNormal);
    const float heightF = mix(heightFF, heightFC, weights.y);
    const float heightC = mix(heightCF, heightCC, weights.y);
    const float height = mix(heightF, heightC, weights.x);

    const float displacement = (height - 0.5) * materialDisplacementIntensity;

    worldPosition += vec4(worldNormal*displacement, 0.0);
	viewPos = (viewMatrix * worldPosition).xyz;
    worldPos = worldPosition.xyz;

    cornerPoint = 0.3*(currentCorners[0]+currentCorners[1]+currentCorners[2]);

    gl_Position = projectionViewMatrix * worldPosition;
}