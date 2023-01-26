#version 450

in vec3 localNormal;
in vec3 worldNormal;
in vec3 scaledLocalPos;

layout (binding = 0) uniform sampler2D borderTex;
layout (location = 3) uniform vec3 backgroundColor = vec3(1.0);

out vec4 fragmentColor;

void main()
{
    vec3 nrmMask = pow(abs(localNormal),vec3(64));
    nrmMask /= nrmMask.x + nrmMask.y + nrmMask.z;
    nrmMask = round(nrmMask);

    float borderXY = texture(borderTex, scaledLocalPos.xy).r;
    float borderXZ = texture(borderTex, scaledLocalPos.xz).r;
    float borderYZ = texture(borderTex, scaledLocalPos.yz).r;

    float border = mix(borderXZ, borderYZ, nrmMask.x);
    border = mix(border, borderXY, nrmMask.z);

    borderXY = texture(borderTex, 5*scaledLocalPos.xy).r;
    borderXZ = texture(borderTex, 5*scaledLocalPos.xz).r;
    borderYZ = texture(borderTex, 5*scaledLocalPos.yz).r;

    float border2 = mix(borderXZ, borderYZ, nrmMask.x);
    border2 = mix(border2, borderXY, nrmMask.z);

    vec3 color = mix(1.0, 0.3, border2).xxx;
    color = mix(color, vec3(0.05), border);
    color *= backgroundColor;

    float lightFactor = max(0.0, dot(
        normalize(vec3(1,1,1)),
        normalize(worldNormal)
    ));
    lightFactor = 0.5 + 0.5*lightFactor;
    lightFactor *= lightFactor;
    color *= lightFactor;

    fragmentColor = vec4(color, 1.0);
}