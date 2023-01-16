#version 450

out vec4 fragmentColor;

layout(location = 0) in vec3 weight;
layout(location = 1) in vec2 center;

//wireframe shader based on https://tchayen.github.io/posts/wireframes-with-barycentric-coordinates
// using smoothstep instead of just step for smooth blending

float edgeFactor()
{
    vec3 d = fwidth(weight);
    vec3 f = smoothstep(weight,vec3(0.0),vec3(d * 0.2));
    return min(min(f.x, f.y), f.z);
}

float fast(vec2 v)
{
    v = (1./4320.) * v + vec2(0.25,0.);
    float state = fract( dot( v * v, vec2(3571)));
    return fract( state * state * (3571. * 2.));
}

void main()
{
    float alpha = 1.0-edgeFactor();
    vec3 color = vec3(0.3+0.4*fast(center));
    color = pow(color,vec3(2.2));
    color = mix(color,alpha.xxx,alpha);
    fragmentColor = vec4(color,1.0);
}