#version 430

out vec4 fragmentColor;

layout (binding = 1) uniform sampler2D normalMap;
layout (binding = 2) uniform sampler2D macroColor;

layout (location = 0) in vec2 uv;
layout (location = 1) flat in vec2 cornerPoint;

// UE4's RandFast function
// https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/Private/Random.ush
//  from: https://www.shadertoy.com/view/XlGcRh
float fast(vec2 v)
{
    v = (1./4320.) * v + vec2(0.25,0.);
    float state = fract( dot( v * v, vec2(3571)));
    return fract( state * state * (3571. * 2.));
}

void main()
{
    float gs = 0.2+0.6*fast(cornerPoint);
    gs = pow(gs,2.2);
    vec3 normal = normalize(2*texture(normalMap, uv).xzy-1);
    vec3 color = texture(macroColor, uv).rgb;
    // vec3 color = vec3(1.0);
    color *= max(dot(normal, normalize(vec3(0.2,1,0.2))), 0.0);
    fragmentColor = vec4(color,1.0);
}