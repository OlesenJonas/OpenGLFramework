#version 430

out vec4 fragmentColor;

layout (location = 1) uniform bool isLinePass;

flat in vec2 cornerPoint;

// UE4's RandFast function
// https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/Private/Random.ush
//  from: https://www.shadertoy.com/view/XlGcRh
float fast(vec2 v)
{
    v = (1./4320.) * v + vec2(0.25,0.);
    float state = fract( dot( v * v, vec2(3571)));
    return fract( state * state * (3571. * 2.));
}

void main() {
    if(isLinePass)
    {
        fragmentColor = vec4(0,0,0,1.0);
    }
    else
    {
        // float gs = 0.2+0.6*hashwithoutsine11(gl_PrimitiveID);
        float gs = 0.2+0.6*fast(cornerPoint);
        fragmentColor = vec4(gs,gs,gs,1.0);

    }
}