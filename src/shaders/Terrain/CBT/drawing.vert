#version 430

layout (location = 0) in vec2 xzPosition;

layout (location = 0) uniform mat4 projectionViewMatrix;

flat out vec2 cornerPoint;

void main()
{
    const vec2[3] corners = vec2[3](
        vec2(-1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0));

    // vec3 worldPosition = vec3(xzPosition.x, 0, xzPosition.y);
    vec3 worldPosition = vec3(           corners[1],0) + 
          xzPosition.x * vec3(corners[2]-corners[1],0) + 
          xzPosition.y * vec3(corners[0]-corners[1],0);

    worldPosition = worldPosition.xzy;

    cornerPoint = worldPosition.xz;
    gl_Position = projectionViewMatrix * vec4(worldPosition, 1);
}