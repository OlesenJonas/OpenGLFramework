#version 450

layout (binding = 0) uniform sampler2D sceneColor;
layout (binding = 1) uniform sampler2D sceneDepth;

layout (location = 0) uniform vec3 cameraPosWS;
layout (location = 1) uniform mat4 invProjView;

uniform vec4 absorptionCoefficient;
uniform vec4 scatteringCoefficient;
uniform vec4 extinctionCoefficient;
uniform vec4 inscatteredLight;
uniform float falloff;
uniform float heightOffset;
uniform vec3 blurTint;

vec3 worldPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posWorldSpace = invProjView * posClipSpace;
    return(posWorldSpace.xyz / posWorldSpace.w);
}

layout (location = 0) out vec4 directLight;
layout (location = 1) out vec4 scatteredLight;

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

    //since theres no light occlusion thats taken into account
    //looking downwards results into the fog without hitting a surface
    //results in a very large amount of inscattered light
    //so just ignore pixels on the bottom hemisphere if no surface visible
    if(pixelDepth == 1 && v.y < 0.0)
    {
        directLight.rgb = pixelColor;
        scatteredLight.rgb = vec3(0.0);
        return;
    }


    // Light camera receives from shaded pixel
    const vec3 sigmaA = absorptionCoefficient.rgb*absorptionCoefficient.w;
    const vec3 sigmaS = scatteringCoefficient.rgb*scatteringCoefficient.w;
    
    //need two integrals, one for scattered and one for absorbed light
    vec3 absorbIntegral = vec3(0);
    vec3 scatterIntegral = vec3(0);
    if(v.y == 0)
    {
        absorbIntegral = D * sigmaA * exp(-origin/falloff);
        scatterIntegral = D * sigmaS * exp(-origin/falloff);
    }
    else
    {          
        const float left = exp((-D*v.y-origin)/falloff)/v.y;
        const float right = exp((-origin)/falloff)/v.y;
        const vec3 ca = -falloff*sigmaA;
        absorbIntegral = ca*left - ca*right;
        const vec3 cs = -falloff*sigmaS;
        scatterIntegral = cs*left - cs*right;
    }
    vec3 transmittanceAbsorption = exp(-absorbIntegral);
    vec3 transmittanceScattering = exp(-scatterIntegral);
    //in my testing the only cases where this results in a NaN is when isinf(Integral), so just set e^-x to 0 in those cases
    //(mix with a bvec doesnt interpolate, it just selects like b?x:y)
    transmittanceAbsorption = mix(transmittanceAbsorption, vec3(0), isnan(transmittanceAbsorption));
    transmittanceScattering = mix(transmittanceScattering, vec3(0), isnan(transmittanceScattering));
    
    const vec3 directLightFromPixel = pixelColor * transmittanceAbsorption * transmittanceScattering;
    const vec3 scatteredLightFromPixel = pixelColor * transmittanceAbsorption * (1-transmittanceScattering);

    const vec3 l_i = inscatteredLight.rgb*inscatteredLight.w;
    const vec3 nonAbsorbedAndNonScatteredInscatteredLight = (1.0 - transmittanceAbsorption*transmittanceScattering) * l_i * (sigmaS/(sigmaA+sigmaS));
    //Calculate inscattered light that isnt absorbed
    const vec3 nonAbsorbedInscatteredLight_NonZeroSigmaA = (1.0 - transmittanceAbsorption) * l_i * (sigmaS/sigmaA);
    vec3 nonAbsorbedInscatteredLight = nonAbsorbedInscatteredLight_NonZeroSigmaA;

    const vec3 directLightFromInscattering = nonAbsorbedAndNonScatteredInscatteredLight;
    const vec3 scatteredLightFromInscattering = nonAbsorbedInscatteredLight-nonAbsorbedAndNonScatteredInscatteredLight;

    directLight.rgb = directLightFromPixel + directLightFromInscattering;
    scatteredLight.rgb = scatteredLightFromPixel*blurTint + scatteredLightFromInscattering;
}