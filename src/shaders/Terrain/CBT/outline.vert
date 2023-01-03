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

void main()
{
    const Node leafNode = leafIndexToNode(gl_InstanceID);
    vec2[3] currentCorners = cornersFromNode(leafNode);

    // corners change winding order every 2nd depth
    if((leafNode.depth & 1u) != 0u)
    {
        // tri subdivision used in paper has the disadvantege of flipping the winding order every level
        currentCorners = vec2[3](currentCorners[2], currentCorners[1], currentCorners[0]);
    }

    centerAndScaleCorners(currentCorners, 500.0);

    // vec3 worldPosition = vec3(xzPosition.x, 0, xzPosition.y);
    vec3 worldPosition = vec3(                  currentCorners[1],0) + 
          xzPosition.x * vec3(currentCorners[2]-currentCorners[1],0) + 
          xzPosition.y * vec3(currentCorners[0]-currentCorners[1],0);

    worldPosition = worldPosition.xzy;
    
    gl_Position = projectionViewMatrix * vec4(worldPosition, 1);
}