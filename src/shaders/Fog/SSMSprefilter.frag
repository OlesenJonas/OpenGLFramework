#version 450

in vec2 passTextureCoord;

layout (binding = 0) uniform sampler2D foggedSceneColor;
layout (binding = 1) uniform sampler2D invTransmittanceTex;

layout (location = 0) uniform vec3 blurTint;

out vec4 fragOut;

// Brightness function
float Brightness(vec3 c)
{
    return max(max(c.r, c.g), c.b);
}

// 3-tap median filter
vec3 Median(vec3 a, vec3 b, vec3 c)
{
    return a + b + c - min(min(a, b), c) - max(max(a, b), c);
}

void main()
{
    const vec2 uv = passTextureCoord;

    vec2 sceneColorTexelSize = 1.0/textureSize(foggedSceneColor,0).xy;

    vec3 d = sceneColorTexelSize.xyx * vec3(1, 1, 0);
    //center pixel
    vec3 s0 = texture(foggedSceneColor, uv).rgb;
    //left
    vec3 s1 = texture(foggedSceneColor, uv - d.xz).rgb;
    //right
    vec3 s2 = texture(foggedSceneColor, uv + d.xz).rgb;
    //bottom
    vec3 s3 = texture(foggedSceneColor, uv - d.zy).rgb;
    //top
    vec3 s4 = texture(foggedSceneColor, uv + d.zy).rgb;

    // vec3 m = Median(Median(s0, s1, s2), s3, s4);
    vec3 m = s0;

    // Pixel brightness
    vec3 br = vec3(Brightness(m));

    // SMSS 
    float invTransmittance = texture(invTransmittanceTex, uv).r; // Deferred
   	// invTransmittance = AdjustDepth(invTransmittance);

    // fragOut.rgb = s0;
    fragOut.rgb = m * invTransmittance * blurTint;
}