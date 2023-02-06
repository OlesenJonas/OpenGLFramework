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

    fragOut.rgb = (s1+s2+s3+s4) * (1.0 / 4);
}