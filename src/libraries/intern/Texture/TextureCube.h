#pragma once

#include <glad/glad/glad.h>
#include <string>
#include <vector>
#include "GLTexture.h"

// forward declared, part of Texture.h
struct TextureDesc;

/** Texture class encapsulating and managing an OpenGL 3D texture object.
 */
class TextureCube : public GLTexture
{
  public:
    /** delete any copy ctor/assign since the implicitly existing ones
     * would only copy the opengl handle and not the full texture
     * meaning if one gets destructed both objects would have an invalid handle
     * and I dont want to deal with that right now
     */
    TextureCube(const TextureCube&) = delete;          // copy constr
    TextureCube& operator=(const TextureCube&) = delete; // copy assign

    /** Creates an immutable Texture object based on a given descriptor.
     * @param descriptor Descriptor to use for configuring the texture
     */
    explicit TextureCube(std::vector<std::string> files);

    /*
        Move constructor

    */
    TextureCube(TextureCube&& other) noexcept;
    /*
        Move assignment
    */
    TextureCube& operator=(TextureCube&& other) noexcept;

    /** Deletes the OpenGL texture object.
     */
    ~TextureCube();

    void swap(TextureCube& other);

    inline int getWidth() const
    {
        return width;
    };

    inline int getHeight() const
    {
        return height;
    };

  private:
    /*
        does not actually store the descriptor (not needed atm)
        if that changes in the future, then make sure that move ctor, move assign and swap
        also move the descriptor
    */
    bool initialized = false;
    int width = -1;
    int height = -1;
};