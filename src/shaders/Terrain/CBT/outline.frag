#version 450

out vec4 fragmentColor;

layout(location = 0) in vec3 weight;

//wireframe shader based on https://tchayen.github.io/posts/wireframes-with-barycentric-coordinates
// using smoothstep instead of just step for smooth blending

float edgeFactor()
{
    vec3 d = fwidth(weight);
    vec3 f = smoothstep(weight,vec3(0.0),vec3(d * 0.25));
    return min(min(f.x, f.y), f.z);
}

void main()
{
    float alpha = 1.0-edgeFactor();
    fragmentColor = vec4(vec3(alpha),alpha*0.5);
}