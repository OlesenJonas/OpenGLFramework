#version 450

layout(location = 3) uniform vec3 color;

out vec4 fragmentColor;

void main()
{
    fragmentColor = vec4(color, 1.0);
}