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

flat out vec2 cornerPoint;

void main()
{
    const Node leafNode = leafIndexToNode(gl_InstanceID);

    //testing culling to square in vertex shader, not really needed atm
    // const uint nodeIndexAtDepth2 = leafNode.heapIndex >> (leafNode.depth - 2);
    // if(nodeIndexAtDepth2 == 4 || nodeIndexAtDepth2 == 7)
    // {
    //     gl_Position.x = 0.0/0.0;
    //     return;
    // }

    vec2[3] currentCorners = cornersFromNode(leafNode);

    centerAndScaleCorners(currentCorners, 500.0);

    // vec3 worldPosition = vec3(xzPosition.x, 0, xzPosition.y);
    vec3 worldPosition = vec3(                  currentCorners[1],0) + 
          xzPosition.x * vec3(currentCorners[2]-currentCorners[1],0) + 
          xzPosition.y * vec3(currentCorners[0]-currentCorners[1],0);

    worldPosition = worldPosition.xzy;

    cornerPoint = 0.3*(currentCorners[0]+currentCorners[1]+currentCorners[2]);

    gl_Position = projectionViewMatrix * vec4(worldPosition, 1);
}