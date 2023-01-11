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

layout (location = 0) uniform mat4 projectionViewMatrix;

layout (binding = 0) uniform sampler2D displacementTex;

#define DISPLACEMENT_ALREADY_DEFINED
#include "transform.glsl"

void main()
{
    const Node leafNode = leafIndexToNode(gl_InstanceID);
    vec2[3] currentCorners = cornersFromNode(leafNode);

    // vec3 worldPosition = vec3(xzPosition.x, 0, xzPosition.y);
    vec2 flatPosition =                     currentCorners[1] + 
          xzPosition.x * (currentCorners[2]-currentCorners[1]) + 
          xzPosition.y * (currentCorners[0]-currentCorners[1]);

    vec4 worldPosition = transformFlatPointToWorldSpace(flatPosition);
    
    gl_Position = projectionViewMatrix * worldPosition;
}