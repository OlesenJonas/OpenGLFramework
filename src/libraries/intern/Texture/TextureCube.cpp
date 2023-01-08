#include <glad/glad/glad.h>

#define _USE_MATH_DEFINES
#include "Texture.h"
#include "TextureCube.h"
#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

TextureCube::TextureCube(const std::vector<std::string>& files)
{
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    // Source: https://learnopengl.com/Advanced-OpenGL/Cubemaps
    int channels;
    for(unsigned int i = 0; i < files.size(); i++)
    {
        unsigned char* data = stbi_load(files[i].c_str(), &width, &height, &channels, 0);
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

static inline glm::vec2 polarCoordinates(const glm::vec3& dir)
{
    const float Pi2 = M_PI * 2.0f;
    const glm::vec3 dirN = glm::normalize(dir);
    const float u = std::max(0.0f, std::min(1.0f, (float)((M_PI + atan2(dirN.x, dirN.z)) / Pi2)));
    const float v = std::max(0.0f, std::min(1.0f, (float)(acos(dirN.y) / M_PI)));
    return glm::vec2(u, acos(dirN.y) / M_PI);
}

TextureCube::TextureCube(const std::string& file)
{
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int channels;
    Color* data = reinterpret_cast<Color*>(stbi_load(file.c_str(), &width, &height, &channels, 0));

    const bool isCubemapFormat = (width * 4 == height * 3) || (width * 3 == height * 4);

    if (isCubemapFormat)
    {
        // Todo: horizontal or vertical cross
        assert(false && "todo: cubemap format not supported");
    }
    else
    {
        // Assume equirectangular

        // Allocate memory for each face
        const int faceSize = 2048;
        const int half = faceSize / 2;
        Color* faces[6];
        for(int i = 0; i < 6; ++i)
        {
            faces[i] = new Color[faceSize * faceSize];
        }

        //TODO: replace point sampling for bilinear

        // Faces X+ & X-
        {
            const float dirZ = (float)half;
            Color* faceA = &faces[0][0];
            Color* faceB = &faces[1][0];
            for(int x = 0; x < faceSize; ++x)
            {
                const float dirY = (float)(x - half);
                for(int y = 0; y < faceSize; ++y)
                {
                    const float dirX = (float)((faceSize - y) - half);
                    {
                        glm::vec2 coord = polarCoordinates({dirX, dirY, dirZ});
                        int pixelX = std::max(std::min((int)(coord.x * (float)width), width - 1), 0);
                        int pixelY = std::max(std::min((int)(coord.y * (float)height), height - 1), 0);
                        *faceA++ = data[pixelX + (pixelY * width)];
                    }
                    {
                        glm::vec2 coord = polarCoordinates({-dirX, dirY, -dirZ});
                        int pixelX = std::max(std::min((int)(coord.x * (float)width), width - 1), 0);
                        int pixelY = std::max(std::min((int)(coord.y * (float)height), height - 1), 0);
                        *faceB++ = data[pixelX + (pixelY * width)];
                    }
                }
            }
        }

        // Faces Y+ & Y-
        {
            const float dirY = (float)half;
            Color* faceA = &faces[2][0];
            Color* faceB = &faces[3][0];
            for(int x = 0; x < faceSize; ++x)
            {
                const float dirX = (float)(x - half);
                for(int y = 0; y < faceSize; ++y)
                {
                    const float dirZ = (float)(y - half);
                    {
                        glm::vec2 coord = polarCoordinates({dirX, -dirY, dirZ});
                        int pixelX = std::max(std::min((int)(coord.x * (float)width), width - 1), 0);
                        int pixelY = std::max(std::min((int)(coord.y * (float)height), height - 1), 0);
                        *faceA++ = data[pixelX + (pixelY * width)];
                    }
                    {
                        glm::vec2 coord = polarCoordinates({-dirX, dirY, -dirZ});
                        int pixelX = std::max(std::min((int)(coord.x * (float)width), width - 1), 0);
                        int pixelY = std::max(std::min((int)(coord.y * (float)height), height - 1), 0);
                        *faceB++ = data[pixelX + (pixelY * width)];
                    }
                }
            }
        }

        // Faces Z+ & Z-
        {
            const float dirX = (float)half;
            Color* faceA = &faces[4][0];
            Color* faceB = &faces[5][0];
            for(int x = 0; x < faceSize; ++x)
            {
                const float dirY = (float)((x) - half);
                for(int y = 0; y < faceSize; ++y)
                {
                    const float dirZ = (float)(y - half);
                    {
                        glm::vec2 coord = polarCoordinates({dirX, dirY, dirZ});
                        int pixelX = std::max(std::min((int)(coord.x * (float)width), width - 1), 0);
                        int pixelY = std::max(std::min((int)(coord.y * (float)height), height - 1), 0);
                        *faceA++ = data[pixelX + (pixelY * width)];
                    }
                    {
                        glm::vec2 coord = polarCoordinates({-dirX, dirY, -dirZ});
                        int pixelX = std::max(std::min((int)(coord.x * (float)width), width - 1), 0);
                        int pixelY = std::max(std::min((int)(coord.y * (float)height), height - 1), 0);
                        *faceB++ = data[pixelX + (pixelY * width)];
                    }
                }
            }
        }

        for(int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, faces[i]);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

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