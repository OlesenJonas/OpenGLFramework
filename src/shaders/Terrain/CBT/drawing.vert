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
layout (location = 2) uniform int materialDisplacementLodOffset = 0;

layout (location = 0) flat out vec2 cornerPoint;
layout (location = 1) out vec2 uv;
layout (location = 2) out vec3 worldPos;

layout(std430, binding = 3) buffer textureInfoBuffer
{   
    float textureScales[];
};

#define DISPLACEMENT_ALREADY_DEFINED
#include "transform.glsl"

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

    // vec3 worldPosition = vec3(xzPosition.x, 0, xzPosition.y);
    vec2 flatPosition =                     currentCorners[1] + 
          xzPosition.x * (currentCorners[2]-currentCorners[1]) + 
          xzPosition.y * (currentCorners[0]-currentCorners[1]);

    uv = vec2(1,-1) * flatPosition + 0.5;

    vec4 worldPosition = transformFlatPointToWorldSpace(flatPosition);
    //pass the *non-displaced* position for triplanar sampling in fragment shader
    worldPos = worldPosition.xyz;

    //sample and transform directly into world space
    // ts=(2*nrmMap-1), ws=(ts.x,ts.z,-ts.y) => ws=(2*n.x-1, 2*n.z-1, -2*n.y+1))
    const vec3 vertexNormal = vec3(2,2,-2) * textureLod(macroNormal,uv,0).xzy + vec3(-1,-1, 1);

    const vec2 scaledUVs = uv*textureSize(materialIDTex,0);
    const ivec2 idsStartTexel = ivec2(scaledUVs);
    const vec2 weights = fract(scaledUVs);

    // TODO: replace with a single textureGather call?
    const uint materialidFF = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(0,0)).r;
    const uint materialidFC = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(0,1)).r;
    const uint materialidCF = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(1,0)).r;
    const uint materialidCC = texelFetchOffset(materialIDTex, idsStartTexel, 0, ivec2(1,1)).r;

    // TODO: Triplanar
    //  Who decides if a sample needs to be triplanar? Store slope information in materialID texture?
    //  KEEP BRANCHING IN MIND! see https://666uille.files.wordpress.com/2017/03/gdc2017_ghostreconwildlands_terrainandtechnologytools-onlinevideos1.pdf
    const float heightFF = textureLod(heightArray, vec3(worldPos.xz/textureScales[materialidFF], materialidFF), materialDisplacementLodOffset).r;
    const float heightFC = textureLod(heightArray, vec3(worldPos.xz/textureScales[materialidFC], materialidFC), materialDisplacementLodOffset).r;
    const float heightCF = textureLod(heightArray, vec3(worldPos.xz/textureScales[materialidCF], materialidCF), materialDisplacementLodOffset).r;
    const float heightCC = textureLod(heightArray, vec3(worldPos.xz/textureScales[materialidCC], materialidCC), materialDisplacementLodOffset).r;

    const float resultF = mix(heightFF, heightFC, weights.y);
    const float resultC = mix(heightCF, heightCC, weights.y);
    const float height = mix(resultF, resultC, weights.x);

    const float displacement = (height - 0.5) * materialDisplacementIntensity;
    worldPosition += vec4(vertexNormal*displacement, 0.0);

    cornerPoint = 0.3*(currentCorners[0]+currentCorners[1]+currentCorners[2]);

    gl_Position = projectionViewMatrix * worldPosition;
}