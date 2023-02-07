#version 430

const float Pi = 3.14159265358979323846;

in vec2 passTextureCoord;

uniform layout (binding = 4) samplerCube tex;
layout(location = 3) uniform int faceIndex;

out vec4 fragmentColor;

const vec3 Forward[6]   = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
const vec3 Right[6]     = {{0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}};
const vec3 Up[6]        = {{0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}};

void main() {

    const vec2 texCoord = vec2(2.0f, -2.0f) * passTextureCoord + vec2(-1.0f, 1.0f);
    const vec3 dir = normalize(Forward[faceIndex] + Right[faceIndex] * texCoord.x + Up[faceIndex] * texCoord.y);

    vec3 irradiance = vec3(0.0f);  

    vec3 up    = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, dir));
    up         = normalize(cross(dir, right));

    // from https://learnopengl.com/PBR/IBL/Diffuse-irradiance:
    float sampleDelta = 0.02f;
    float nrSamples = 0.0; 
    for(float phi = 0.0; phi < 2.0 * Pi; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * Pi; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * dir; 

            irradiance += max(vec3(0,0,0), textureLod(tex, sampleVec, 0).rgb) * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    fragmentColor.xyz = Pi * irradiance * (1.0 / float(nrSamples));
}