#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

#ifndef MATERIAL_ATTRIBUTES_GLSL
#define MATERIAL_ATTRIBUTES_GLSL

struct MaterialAttributes
{
    vec4 diffuseRoughness;
    vec4 normalMetallic;
    vec2 aoHeight;
};

MaterialAttributes CreateMaterialAttributes(const vec3 diffuse, const float roughness, const vec3 normal, const float metallic, const float ao, const float height)
{
    return MaterialAttributes(
        vec4(diffuse, roughness),
        vec4(normal, metallic),
        vec2(ao, height)
    );
}

MaterialAttributes CreateMaterialAttributes(const vec3 diffuse, const vec3 normal, const vec3 ord)
{
    return MaterialAttributes(
        vec4(diffuse, ord.g),
        vec4(normal, 0.0),
        vec2(ord.r, ord.b)
    );
}

MaterialAttributes add(const MaterialAttributes a, const MaterialAttributes b)
{
    return MaterialAttributes(
        a.diffuseRoughness + b.diffuseRoughness,
        a.normalMetallic + b.normalMetallic,
        a.aoHeight + b.aoHeight
    );
}

MaterialAttributes multiply(const MaterialAttributes attributes, const float factor)
{
    return MaterialAttributes(
        attributes.diffuseRoughness * factor,
        attributes.normalMetallic * factor,
        attributes.aoHeight * factor
    );
}

MaterialAttributes divide(const MaterialAttributes attributes, const float factor)
{
    return MaterialAttributes(
        attributes.diffuseRoughness / factor,
        attributes.normalMetallic / factor,
        attributes.aoHeight / factor
    );
}

MaterialAttributes lerp(const MaterialAttributes a, const MaterialAttributes b, float factor)
{
    return MaterialAttributes(
        mix(a.diffuseRoughness, b.diffuseRoughness, factor),
        mix(a.normalMetallic, b.normalMetallic, factor),
        mix(a.aoHeight, b.aoHeight, factor)
    );
}

#endif