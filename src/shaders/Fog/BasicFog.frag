#version 450

layout (binding = 0) uniform sampler2D sceneColor;
layout (binding = 1) uniform sampler2D sceneDepth;

layout (location = 0) uniform vec3 cameraPosWS;
layout (location = 1) uniform mat4 invProjView;

layout (location = 2) uniform vec4 absorptionCoefficient;
layout (location = 3) uniform vec4 scatteringCoefficient;
layout (location = 8) uniform vec4 extinctionCoefficient;
// assume inscattered light is equal everywhere! this is the reason this is called "BasicFog"
layout (location = 4) uniform vec4 inscatteredLight;
layout (location = 5) uniform float falloff;
layout (location = 6) uniform float heightOffset;
layout (location = 7) uniform int mode;

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

    const vec3 sigmaA = absorptionCoefficient.rgb*absorptionCoefficient.w;
    const vec3 sigmaS = scatteringCoefficient.rgb*scatteringCoefficient.w;

    vec3 pixelColor = texelFetch(sceneColor, ivec2(gl_FragCoord.xy),0).rgb;

    vec3 d = pixelPosWorldSpace - cameraPosWS;
    float D = length(d);
    d = d/D;
    const float origin = cameraPosWS.y + heightOffset;

    if(mode == 0)
    {
        const vec3 sigmaT = extinctionCoefficient.rgb*extinctionCoefficient.w;

        // Light camera receives from shaded pixel
        vec3 fogIntegral = sigmaT * exp(-origin/falloff);
        if(d.y == 0) //very unlikely that its exactly 0
        {
            fogIntegral *= D;
        }
        else
        {
            const float exponent = -D*d.y/falloff;
            const float inner = 1.0 - exp(exponent);
            fogIntegral *= falloff*inner/d.y;
        }
        vec3 transmittance = exp(-fogIntegral);
        // vec3 transmittance = max(vec3(0), exp(-fogIntegral));
        pixelColor *= transmittance;

        //simple 1-transmittance
        pixelColor += inscatteredLight.rgb*inscatteredLight.w * (1-transmittance);
    }
    //TODO: apply change in formula!
    else
    {
        // Light camera receives from shaded pixel
        const float fogFactorAtCamera = exp(-falloff * origin);
        vec3 fogIntegral = (sigmaA+sigmaS) * fogFactorAtCamera;
        //todo: needed? just to prevent NaNs?
        const float verticalLookThreshold = 0.01;
        // if(abs(d.y) > verticalLookThreshold)
        {
            const float t = falloff * d.y;
            fogIntegral *= ( 1.0 - exp(-D*t) ) / t;
        }
        // vec3 transmittance = exp(-fogIntegral);
        vec3 transmittance = max(vec3(0), exp(-fogIntegral));
        pixelColor *= transmittance;

        // Light the camera receives from all the points between pixel and camera
        vec3 inScatterIntegral = inscatteredLight.rgb*inscatteredLight.w * sigmaS / (sigmaS+sigmaA);
        vec3 exponent = (sigmaA + sigmaS) * (exp(falloff*origin) - exp(falloff*(D*d.y+origin))) * exp(-falloff*(D*d.y+2*origin));
        // inScatterIntegral *= 1-exp(exponent/(falloff*d.y));
        inScatterIntegral *= max(vec3(0),1-exp(exponent/(falloff*d.y)));
        //add integral
        pixelColor += inScatterIntegral;
    }

    fragmentColor = vec4(pixelColor,1.0);

    // fragmentColor.rgb = fract(pixelPosWorldSpace);
    // fragmentColor.a = 1.0;
}