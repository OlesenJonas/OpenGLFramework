#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textureCoord;
layout (location = 3) in vec4 tangent;

layout (location = 0) uniform mat4 modelMatrix;
layout (location = 1) uniform mat4 viewMatrix;
layout (location = 2) uniform mat4 projectionMatrix;

out vec2 oUv;
out vec3 oNormal;
out vec4 oViewPosition;
out mat3 oTangentFrame;

void main()
{
    oViewPosition = viewMatrix * modelMatrix * position;
    gl_Position = projectionMatrix * oViewPosition;
    oUv = textureCoord;
    oNormal = (modelMatrix * vec4(normal, 0)).xyz;
    
    vec3 bitangent = cross(normal, tangent.xyz);
    vec3 T = normalize(vec3((modelMatrix * vec4(tangent.xyz, 0)).xyz));
    vec3 B = normalize(vec3((modelMatrix * vec4(bitangent, 0)).xyz));
    vec3 N = normalize(vec3((modelMatrix * vec4(normal, 0)).xyz));
    oTangentFrame = mat3(T, B, N);

}