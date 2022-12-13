#version 430

layout (location = 0) in vec2 xzPosition;

layout (location = 0) uniform mat4 projectionViewMatrix;

flat out vec2 cornerPoint;

void main()
{
    cornerPoint = xzPosition;
    gl_Position = projectionViewMatrix * vec4(xzPosition.x, 0, xzPosition.y, 1);
}