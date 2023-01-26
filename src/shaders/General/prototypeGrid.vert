#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;

layout (location = 0) uniform mat4 modelMatrix;
layout (location = 1) uniform mat4 viewMatrix;
layout (location = 2) uniform mat4 projectionMatrix;

out vec3 localNormal;
out vec3 worldNormal;
out vec3 scaledLocalPos;

void main()
{
    localNormal = normal;

    worldNormal = normalize(vec3(transpose(inverse(modelMatrix)) * vec4(normal, 0.0)));

    vec3 localScale = vec3(
        length((modelMatrix)*vec4(1,0,0,0)),
        length((modelMatrix)*vec4(0,1,0,0)),
        length((modelMatrix)*vec4(0,0,1,0))
    );
    scaledLocalPos = position.xyz * localScale;

    gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;
}