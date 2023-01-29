#pragma once

#include <glm/glm.hpp>

// template <typename T>
// using sRGBboolType = bool;
// "Explicit specialization of alias templates is not permitted", so use verbose struct workaround instead
template <typename T>
struct sRGBTypeHelper
{
    using boolType = void;
    using noAlphaType = void;
};
template <>
struct sRGBTypeHelper<glm::vec4>
{
    using boolType = glm::bvec3;
    using noAlphaType = glm::vec3;
};
template <>
struct sRGBTypeHelper<glm::vec3>
{
    using boolType = glm::bvec3;
    using noAlphaType = glm::vec3;
};
template <>
struct sRGBTypeHelper<glm::vec2>
{
    using boolType = glm::bvec2;
    using noAlphaType = glm::vec2;
};

// from https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
//  Converts a color from linear light gamma to sRGB gamma
template <typename T>
constexpr T sRGBfromLinear(const T& linearRGB)
{
    using boolType = typename sRGBTypeHelper<T>::boolType;
    using colorType = typename sRGBTypeHelper<T>::noAlphaType;

    const colorType linearRGBrgb{linearRGB};
    const boolType cutoff = glm::lessThan(linearRGBrgb, colorType(0.0031308f));
    const colorType higher =
        colorType(1.055f) * glm::pow(linearRGBrgb, colorType(1.0f / 2.4f)) - colorType(0.055f);
    const colorType lower = linearRGBrgb * colorType(12.92f);

    if constexpr(std::is_same_v<T, glm::vec4>)
    {
        return {glm::mix(higher, lower, cutoff), linearRGB.a};
    }
    else
    {
        return {glm::mix(higher, lower, cutoff)};
    }
}

template <typename T>
constexpr T sRGBtoLinear(const T& sRGB)
{
    using boolType = typename sRGBTypeHelper<T>::boolType;
    using colorType = typename sRGBTypeHelper<T>::noAlphaType;

    const colorType sRGBrgb{sRGB};
    const boolType cutoff = lessThan(sRGBrgb, colorType(0.04045f));
    const colorType higher = glm::pow((sRGBrgb + colorType(0.055f)) / colorType(1.055f), colorType(2.4f));
    const colorType lower = sRGBrgb / colorType(12.92f);

    if constexpr(std::is_same_v<T, glm::vec4>)
    {
        return {glm::mix(higher, lower, cutoff), sRGB.a};
    }
    else
    {
        return {glm::mix(higher, lower, cutoff)};
    }
}

template <>
float sRGBfromLinear(const float& linearR)
{
    const bool cutoff = linearR < 0.0031308f;
    const float higher = 1.055f * glm::pow(linearR, 1.0f / 2.4f) - 0.055f;
    const float lower = linearR * 12.92f;

    return glm::mix(higher, lower, cutoff);
}

template <>
float sRGBtoLinear(const float& sR)
{
    const bool cutoff = sR < 0.04045f;
    const float higher = pow((sR + 0.055f) / 1.055f, 2.4f);
    const float lower = sR / 12.92f;

    return glm::mix(higher, lower, cutoff);
}