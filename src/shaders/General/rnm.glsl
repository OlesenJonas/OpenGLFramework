#ifndef RNM_GLSL
#define RNM_GLSL

// Reorient normal blending for blending material normal maps and terrain normal:
// https://blog.selfshadow.com/publications/blending-in-detail/
vec3 reorientNormalBlend(vec3 t, vec3 u)
{
    t += vec3(0,0,1);
    u *= vec3(-1,-1,1);
    return normalize(t*dot(t, u) - u*t.z);
}

#endif
