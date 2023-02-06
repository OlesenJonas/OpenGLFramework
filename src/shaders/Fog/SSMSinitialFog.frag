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
uniform bool doFade;
uniform float fadeStart;
uniform float fadeLength;
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


    // Light camera receives from shaded pixel
    const vec3 sigmaA = absorptionCoefficient.rgb*absorptionCoefficient.w;
    const vec3 sigmaS = scatteringCoefficient.rgb*scatteringCoefficient.w;
    
    //need two integrals, one for scattered and one for absorbed light
    vec3 absorbIntegral = vec3(0);
    vec3 scatterIntegral = vec3(0);
    if(!doFade)
    {   
        //if no fade is happening, we only need to solve a single integral
        if(v.y == 0) //very unlikely that its exactly 0
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
    }
    else
    {
        //See ScalingDensityByCameraDistance.ipynb

        const float a = fadeStart;
        const float b = fadeStart+fadeLength;
        
        //TODO: not really optimized at all
        if(D > a)
        {
            //First Integral
            // always evaluates to 0

            //2nd Integral
            // if(v.y == 0)
            if(abs(v.y) <= 0.001)
            {
                //TODO: the 2nd branch could be expanded and would then equal the first, removing the branch altogether
                if(b > D)
                {
                    absorbIntegral += (sigmaA*(-D*D + 2*D*a - a*a)*exp(-origin/falloff))/(2*(a-b));
                    scatterIntegral += (sigmaS*(-D*D + 2*D*a - a*a)*exp(-origin/falloff))/(2*(a-b));
                }
                else
                {
                    absorbIntegral += sigmaA*(b-a)*exp(-origin/falloff)*0.5;
                    scatterIntegral += sigmaS*(b-a)*exp(-origin/falloff)*0.5;
                }
            }
            else
            {
                const float end = min(b,D);
                const float left = -falloff * exp(-(a*v.y+origin)/falloff);
                const float right = (end*v.y - a*v.y + falloff)*exp(-(end*v.y+origin)/falloff);
                const vec3 numeratorA = falloff*sigmaA*(left+right);
                const vec3 numeratorS = falloff*sigmaS*(left+right);
                absorbIntegral += numeratorA/(v.y*v.y*(a-b));
                scatterIntegral += numeratorS/(v.y*v.y*(a-b));
            }

            //3rd Integral
            if(b < D)
            {
                //if b >= D the third Integral isnt needed

                if(v.y == 0)
                {
                    absorbIntegral += sigmaA*(D-b)*exp(-origin/falloff);
                    scatterIntegral += sigmaS*(D-b)*exp(-origin/falloff);
                }
                else
                {
                    const float left  = exp((-D*v.y-origin)/falloff)/v.y;
                    const float right = exp((-b*v.y-origin)/falloff)/v.y;
                    const vec3 ca = falloff*sigmaA;
                    const vec3 cs = falloff*sigmaS;
                    absorbIntegral += -ca*left + ca*right;
                    scatterIntegral += -cs*left + cs*right;
                }
            }
        }
    }
    vec3 transmittanceAbsorption = exp(-absorbIntegral);
    vec3 transmittanceScattering = exp(-scatterIntegral);
    //in my testing the only cases where this results in a NaN is when isinf(Integral), so just set e^-x to 0 in those cases
    //(mix with a bvec doesnt interpolate, it just selects like b?x:y)
    transmittanceAbsorption = mix(transmittanceAbsorption, vec3(0), isnan(transmittanceAbsorption));
    transmittanceScattering = mix(transmittanceScattering, vec3(0), isnan(transmittanceScattering));
    
    const vec3 directLightFromPixel = pixelColor * transmittanceAbsorption * transmittanceScattering;
    const vec3 scatteredLightFromPixel = pixelColor * transmittanceAbsorption * (1-transmittanceScattering);

    // Light the camera receives from all the points between pixel and camera (with uniform inscattering)
    const vec3 l_i = inscatteredLight.rgb*inscatteredLight.w;
    const vec3 albedo = sigmaS/(sigmaA+sigmaS);
    const vec3 lightFromInscattering = (1.0 - transmittanceAbsorption*transmittanceScattering) * l_i * albedo;
    
    // const vec3 factor
    //TODO: mix part of inscattering light into indirect image

    directLight.rgb = directLightFromPixel + lightFromInscattering;

    scatteredLight.rgb = scatteredLightFromPixel*blurTint;
}