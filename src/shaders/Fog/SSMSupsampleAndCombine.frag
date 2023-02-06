#version 450

in vec2 passTextureCoord;

layout (binding = 0) uniform sampler2D fullSizeTex;
layout (binding = 1) uniform sampler2D halfSizeUpsampledTex;

layout (location = 0) uniform float sampleScale;
layout (location = 1) uniform float steps;
layout (location = 2) uniform float intensity;

out vec4 fragOut;

void main()
{
    const vec2 uv = passTextureCoord;

    vec3 base = texture(fullSizeTex, uv).rgb;

    // 9-tap bilinear upsampler (tent filter)
    vec4 d = (1.0/textureSize(halfSizeUpsampledTex,0)).xyxy * vec4(1, 1, -1, 0) * sampleScale;

    vec3 s;
    s  = texture(halfSizeUpsampledTex, uv - d.xy).rgb;
    s += texture(halfSizeUpsampledTex, uv - d.wy).rgb * 2;
    s += texture(halfSizeUpsampledTex, uv - d.zy).rgb;

    s += texture(halfSizeUpsampledTex, uv + d.zw).rgb * 2;
    s += texture(halfSizeUpsampledTex, uv       ).rgb * 4;
    s += texture(halfSizeUpsampledTex, uv + d.xw).rgb * 2;

    s += texture(halfSizeUpsampledTex, uv + d.zy).rgb;
    s += texture(halfSizeUpsampledTex, uv + d.wy).rgb * 2;
    s += texture(halfSizeUpsampledTex, uv + d.xy).rgb;

    vec3 blur = s * (1.0 / 16);

    fragOut.rgb = base + intensity * blur * (1 / steps);
}