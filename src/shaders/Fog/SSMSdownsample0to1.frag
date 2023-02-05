#version 450

in vec2 passTextureCoord;

layout (binding = 0) uniform sampler2D levelAbove;

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

    vec4 d = (1.0/textureSize(levelAbove,0)).xyxy * vec4(-1, -1, +1, +1);

    vec3 s1 = texture(levelAbove, uv + d.xy).rgb;
    vec3 s2 = texture(levelAbove, uv + d.zy).rgb;
    vec3 s3 = texture(levelAbove, uv + d.xw).rgb;
    vec3 s4 = texture(levelAbove, uv + d.zw).rgb;

    // Karis's luma weighted average (using brightness instead of luma)
    float s1w = 1 / (Brightness(s1) + 1);
    float s2w = 1 / (Brightness(s2) + 1);
    float s3w = 1 / (Brightness(s3) + 1);
    float s4w = 1 / (Brightness(s4) + 1);
    float one_div_wsum = 1 / (s1w + s2w + s3w + s4w);

    fragOut.rgb = (s1 * s1w + s2 * s2w + s3 * s3w + s4 * s4w) * one_div_wsum;
}