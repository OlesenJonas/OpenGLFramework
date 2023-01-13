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

layout (location = 0) uniform float aspect;

layout (location = 0) out vec2 center;

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

    vec2 worldPosition =                    currentCorners[1] + 
          xzPosition.x * (currentCorners[2]-currentCorners[1]) + 
          xzPosition.y * (currentCorners[0]-currentCorners[1]);
    
    worldPosition.y *= -1;
    worldPosition += 0.5;

    worldPosition.x /= aspect;
    worldPosition /= 2.0;
    
    worldPosition -= 1.0;

    gl_Position = vec4(worldPosition, 0, 1);
    center = (currentCorners[0]+currentCorners[1]+currentCorners[2])/3.0;
}