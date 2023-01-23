#version 450

layout (binding = 0) uniform sampler2D sceneColor;
layout (binding = 1) uniform sampler2D sceneDepth;

layout (location = 0) uniform vec3 cameraPosWS;
layout (location = 1) uniform mat4 invProjView;

vec3 worldPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posWorldSpace = invProjView * posClipSpace;
    return(posWorldSpace.xyz / posWorldSpace.w);
}

out vec4 fragmentColor;

void main() {
    
    float pixelDepth = texelFetch(sceneDepth, ivec2(gl_FragCoord.xy),0).r;
    vec2 screenUV = gl_FragCoord.xy/vec2(textureSize(sceneColor,0));
    vec3 pixelPosWorldSpace = worldPositionFromDepth(screenUV, pixelDepth);

    const float sigmaT = 0.5;
    const float falloff = .1;

    // Height fog integral from
    // Real-time Atmospheric Effects in Games Revisited, Carsten Wenzel, Crytek
    const vec3 d = pixelPosWorldSpace - cameraPosWS;
    const float fogFactorAtCamera = exp(-falloff * cameraPosWS.y);
    float fogIntegral = length(d) * fogFactorAtCamera;

    //todo: needed? just to prevent NaNs?
    const float verticalLookThreshold = 0.01;
    // if(abs(d.y) > verticalLookThreshold)
    {
        const float t = falloff * d.y;
        fogIntegral *= ( 1.0 - exp(-t) ) / t;
    }
    float transmittance = exp(-sigmaT * fogIntegral);

    fragmentColor = texelFetch(sceneColor, ivec2(gl_FragCoord.xy),0) * transmittance;

    // fragmentColor.rgb = fract(pixelPosWorldSpace);
    // fragmentColor.a = 1.0;
}