#include <glad/glad/glad.h>

#include "Texture.h"
#include "TextureCube.h"

TextureCube::TextureCube(std::vector<std::string> files)
{
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    // Source: https://learnopengl.com/Advanced-OpenGL/Cubemaps
    int nrChannels;
    for(unsigned int i = 0; i < files.size(); i++)
    {
        unsigned char* data = stbi_load(files[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << files[i] << std::endl;
        }
        stbi_image_free(data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    initialized = true;
}

TextureCube::TextureCube(TextureCube&& other) noexcept
    : width(other.width), height(other.height)
{
    // if this texture was already initialized then somebody needs to delete the old content
    // so swap IDs and init staus instead of just moving and *other will delete on destruction
    std::swap(textureID, other.textureID);
    std::swap(initialized, other.initialized);
}

TextureCube& TextureCube::operator=(TextureCube&& other) noexcept
{
    width = other.width;
    height = other.height;
    // if this texture was already initialized then somebody needs to delete the old content
    // so swap IDs and init staus instead of just moving and *other will delete on destruction
    std::swap(textureID, other.textureID);
    std::swap(initialized, other.initialized);
    return *this;
}

TextureCube::~TextureCube()
{
    if(initialized)
    {
        glDeleteTextures(1, &textureID);
    }
}

void TextureCube::swap(TextureCube& other)
{
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(textureID, other.textureID);
    std::swap(initialized, other.initialized);
}