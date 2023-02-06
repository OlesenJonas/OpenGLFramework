#version 450

in vec2 passTextureCoord;

layout (binding = 0) uniform sampler2D nextLevelTex; //1 mip level higher, the texture to be upsampled

layout (location = 0) uniform float sampleScale;
layout (location = 1) uniform float blurWeight;

out vec4 fragOut;

void main()
{
    const vec2 uv = passTextureCoord;

    // 9-tap bilinear upsampler (tent filter)
    vec4 d = (1.0/textureSize(nextLevelTex,0)).xyxy * vec4(1, 1, -1, 0) * sampleScale;

    vec3 s;
    s  = texture(nextLevelTex, uv - d.xy).rgb;
    s += texture(nextLevelTex, uv - d.wy).rgb * 2;
    s += texture(nextLevelTex, uv - d.zy).rgb;

    s += texture(nextLevelTex, uv + d.zw).rgb * 2;
    s += texture(nextLevelTex, uv       ).rgb * 4;
    s += texture(nextLevelTex, uv + d.xw).rgb * 2;

    s += texture(nextLevelTex, uv + d.zy).rgb;
    s += texture(nextLevelTex, uv + d.wy).rgb * 2;
    s += texture(nextLevelTex, uv + d.xy).rgb;

    vec3 upsampledBlur = s * (1.0 / 16);

    // fragOut.rgb = (base + upsampledBlur * (1 + blurWeight)) / (1 + (blurWeight * 0.735));
    float scaleFactor = 1.0 / (1.0 + (blurWeight * 0.735));
    fragOut.rgb = upsampledBlur * (1.0 + blurWeight);
    fragOut.w = scaleFactor;
}