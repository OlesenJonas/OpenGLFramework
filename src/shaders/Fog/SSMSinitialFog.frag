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
uniform bool doFade;
uniform float fadeStart;
uniform float fadeLength;

vec3 worldPositionFromDepth(vec2 texCoord, float depthBufferDepth)
{
    vec4 posClipSpace = vec4(texCoord * 2.0 - vec2(1.0), 2.0 * depthBufferDepth - 1.0, 1.0);
    vec4 posWorldSpace = invProjView * posClipSpace;
    return(posWorldSpace.xyz / posWorldSpace.w);
}

layout (location = 0) out vec4 sceneColorWithFog;
layout (location = 1) out vec4 invTransmittance;

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
    const vec3 sigmaT = sigmaA+sigmaS;
    
    vec3 fogIntegral = vec3(0);
    if(!doFade)
    {   
        //if no fade is happening, we only need to solve a single integral
        if(v.y == 0) //very unlikely that its exactly 0
        {
            fogIntegral = D * sigmaT * exp(-origin/falloff);
        }
        else
        {          
            const vec3 cas = -falloff*sigmaT;
            const vec3 left = cas*exp((-D*v.y-origin)/falloff)/v.y;
            const vec3 right = cas*exp((-origin)/falloff)/v.y;
            fogIntegral = left - right;
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
                    fogIntegral += (sigmaT*(-D*D + 2*D*a - a*a)*exp(-origin/falloff))/(2*(a-b));
                }
                else
                {
                    fogIntegral += sigmaT*(b-a)*exp(-origin/falloff)*0.5;
                }
            }
            else
            {
                const float end = min(b,D);
                const float left = -falloff * exp(-(a*v.y+origin)/falloff);
                const float right = (end*v.y - a*v.y + falloff)*exp(-(end*v.y+origin)/falloff);
                const vec3 numerator = falloff*sigmaT*(left+right);
                fogIntegral += numerator/(v.y*v.y*(a-b));
            }

            //3rd Integral
            if(b < D)
            {
                //if b >= D the third Integral isnt needed

                if(v.y == 0)
                {
                    fogIntegral += sigmaT*(D-b)*exp(-origin/falloff);
                }
                else
                {
                    const vec3 cst = falloff*sigmaT;
                    const vec3 left  = cst*exp((-D*v.y-origin)/falloff)/v.y;
                    const vec3 right = cst*exp((-b*v.y-origin)/falloff)/v.y;
                    fogIntegral += -left + right;
                }
            }
        }
    }
    vec3 transmittance = exp(-fogIntegral);
    //in my testing the only cases where this results in a NaN is when isinf(fogIntegral), so just set e^-x to 0 in those cases
    //(mix with a bvec doesnt interpolate, it just selects like b?x:y)
    transmittance = mix(transmittance, vec3(0), isnan(transmittance));
    pixelColor *= transmittance;

    // Light the camera receives from all the points between pixel and camera (with uniform inscattering)
    const vec3 l_i = inscatteredLight.rgb*inscatteredLight.w;
    const vec3 albedo = sigmaS/sigmaT;
    vec3 inScatterIntegral = (1.0-transmittance)*l_i*albedo;

    //add integral
    pixelColor += inScatterIntegral;

    sceneColorWithFog = vec4(pixelColor,1.0);
    invTransmittance.rgb = (1.0 - transmittance);
}