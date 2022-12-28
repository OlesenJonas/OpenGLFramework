#version 450

layout (location = 0) in vec2 xzPosition;

layout (location = 0) uniform mat4 projectionViewMatrix;

flat out vec2 cornerPoint;

void main()
{
    const vec2 A = vec2(-1 ,-1);
    const vec2 B = vec2(-1 , 1);
    const vec2 C = vec2( 1 , 1);

    // const vec2[2][3] corners = vec2[2][3](
    //     vec2[3](A,0.5*(A+C),B),
    //     vec2[3](B,0.5*(A+C),C)
    // );

    //depth = 1 -> uneven -> order is flipped
    //  flip local "A" and "C" (DONT TOUCH B, OTHERWISE TEMPLATE MESH SUBDIV DOESNT WORK ANYMORE)

    const vec2[2][3] corners = vec2[2][3](
        vec2[3](B,0.5*(A+C),A),
        vec2[3](C,0.5*(A+C),B)
    );

    const vec2[3] currentCorners = corners[gl_InstanceID];

    // vec3 worldPosition = vec3(xzPosition.x, 0, xzPosition.y);
    vec3 worldPosition = vec3(                  currentCorners[1],0) + 
          xzPosition.x * vec3(currentCorners[2]-currentCorners[1],0) + 
          xzPosition.y * vec3(currentCorners[0]-currentCorners[1],0);

    worldPosition = worldPosition.xzy;

    cornerPoint = worldPosition.xz;
    gl_Position = projectionViewMatrix * vec4(worldPosition, 1);
}