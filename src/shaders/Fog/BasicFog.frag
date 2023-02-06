#version 450

layout (binding = 0) uniform sampler2D sceneColor;
layout (binding = 1) uniform sampler2D sceneDepth;

layout (location = 0) uniform vec3 cameraPosWS;
layout (location = 1) uniform mat4 invProjView;

uniform vec4 absorptionCoefficient;
uniform vec4 scatteringCoefficient;
uniform vec4 extinctionCoefficient;
// assume inscattered light is equal, one of the reasons this is just basicFog
uniform vec4 inscatteredLight;
uniform float falloff;
uniform float heightOffset;

vec3 worldPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posWorldSpace = invProjView * posClipSpace;
    return(posWorldSpace.xyz / posWorldSpace.w);
}

out vec4 fragmentColor;

void main()
{    
    float pixelDepth = texelFetch(sceneDepth, ivec2(gl_FragCoord.xy),0).r;
    vec2 screenUV = gl_FragCoord.xy/vec2(textureSize(sceneColor,0));
    vec3 pixelPosWorldSpace = worldPositionFromDepth(screenUV, pixelDepth);

    vec3 pixelColor = texelFetch(sceneColor, ivec2(gl_FragCoord.xy),0).rgb;

    vec3 v = pixelPosWorldSpace - cameraPosWS;
    float D = length(v);
    v = v/D;
    const vec3 startPos = cameraPosWS;
    const float origin = startPos.y + heightOffset;

    const vec3 sigmaA = absorptionCoefficient.rgb*absorptionCoefficient.w;
    const vec3 sigmaS = scatteringCoefficient.rgb*scatteringCoefficient.w;
    const vec3 sigmaT = sigmaA+sigmaS;
    
    vec3 fogIntegral = vec3(0);
 
    if(v.y == 0)
    {
        fogIntegral = D * sigmaT * exp(-origin/falloff);
    }
    else
    {          
        const vec3 ct = falloff*sigmaT;
        const vec3 left = ct*exp((-D*v.y-origin)/falloff)/v.y;
        const vec3 right = ct*exp((-origin)/falloff)/v.y;
        fogIntegral = -left + right;
    }
    vec3 transmittance = exp(-fogIntegral);
    // transmittance = mix(transmittance, vec3(0), isinf(fogIntegral));
    transmittance = mix(transmittance, vec3(0), isnan(transmittance));
    pixelColor *= transmittance;

    // Light the camera receives from all the points between pixel and camera (with uniform inscattering)
    const vec3 l_i = inscatteredLight.rgb*inscatteredLight.w;
    const vec3 albedo = sigmaS/sigmaT;
    vec3 inScatterIntegral = l_i*albedo - transmittance*l_i*albedo;

    //add integral
    pixelColor += inScatterIntegral;

    fragmentColor = vec4(pixelColor,1.0);

    // fragmentColor.rgb = fract(pixelPosWorldSpace);
    // fragmentColor.a = 1.0;
}