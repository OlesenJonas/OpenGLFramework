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
uniform int mode;

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

    vec3 d = pixelPosWorldSpace - cameraPosWS;
    float D = length(d);
    d = d/D;
    const float origin = cameraPosWS.y + heightOffset;

    //todo: could add mode branch just around the parts that need it, would shorten file a bit
    if(mode == 0)
    {
        // Light camera receives from shaded pixel

        const vec3 sigmaT = extinctionCoefficient.rgb*extinctionCoefficient.w;
        vec3 fogIntegral;
        if(d.y == 0) //very unlikely that its exactly 0
        {
            fogIntegral = D * sigmaT * exp(-origin/falloff);
        }
        else
        {          
            const vec3 cas = -falloff*sigmaT;
            const vec3 left = cas*exp((-D*d.y-origin)/falloff)/d.y;
            const vec3 right = cas*exp((-origin)/falloff)/d.y;
            fogIntegral = left - right;
        }
        vec3 transmittance = exp(-fogIntegral);
        //in my testing the only cases where this results in a NaN is when isinf(fogIntegral), so just set e^-x to 0 in those cases
        //(mix with a bvec doesnt interpolate, it just selects like b?x:y)
        transmittance = mix(transmittance, vec3(0), isnan(transmittance));
        pixelColor *= transmittance;

        //simple 1-transmittance
        pixelColor += inscatteredLight.rgb*inscatteredLight.w * (1-transmittance);
    }
    else if(mode == 1)
    {
        // Light camera receives from shaded pixel

        const vec3 sigmaA = absorptionCoefficient.rgb*absorptionCoefficient.w;
        const vec3 sigmaS = scatteringCoefficient.rgb*scatteringCoefficient.w;
        const vec3 sigmaT = sigmaA+sigmaS;
        
        vec3 fogIntegral;
        if(d.y == 0) //very unlikely that its exactly 0
        {
            fogIntegral = D * sigmaT * exp(-origin/falloff);
        }
        else
        {          
            const vec3 cas = -falloff*sigmaT;
            const vec3 left = cas*exp((-D*d.y-origin)/falloff)/d.y;
            const vec3 right = cas*exp((-origin)/falloff)/d.y;
            fogIntegral = left - right;
        }
        vec3 transmittance = exp(-fogIntegral);
        transmittance = mix(transmittance, vec3(0), isnan(transmittance));
        pixelColor *= transmittance;

        // Light the camera receives from all the points between pixel and camera (with uniform inscattering)
        const vec3 l_i = inscatteredLight.rgb*inscatteredLight.w;
        const vec3 albedo = sigmaS/sigmaT;
        vec3 inScatterIntegral = l_i*albedo - transmittance*l_i*albedo;

        //add integral
        pixelColor += inScatterIntegral;
    }

    fragmentColor = vec4(pixelColor,1.0);

    // fragmentColor.rgb = fract(pixelPosWorldSpace);
    // fragmentColor.a = 1.0;
}