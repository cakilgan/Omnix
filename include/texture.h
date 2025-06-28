#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <glad/gl.h>
#include <stb_image.h>
#include <iostream>

class Texture {
public:
    Texture() : textureID(0), width(0), height(0), channels(0) {}

    ~Texture() {
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
        }
    }

    bool loadFromFile(const std::string& path, bool flipVertically = true,
                      max::vec4<unsigned char> force_data_RGBA = {}, max::vec4<unsigned char> force_data_VALUE = {}) {
        if (flipVertically) {
            stbi_set_flip_vertically_on_load(1);
        }
    
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            std::cerr << "Failed to load image: " << path << std::endl;
            return false;
        }
    
        channels = 4; 
        if (force_data_RGBA.x + force_data_RGBA.y + force_data_RGBA.z > 0) {
            for (int i = 0; i < width * height * 4; i += 4) {
                unsigned char r = data[i];
                unsigned char g = data[i + 1];
                unsigned char b = data[i + 2];
                if (r == force_data_RGBA.x && g == force_data_RGBA.y && b == force_data_RGBA.z) {
                    data[i] = force_data_VALUE.x; 
                    data[i + 1] = force_data_VALUE.y; 
                    data[i + 2] = force_data_VALUE.z; 
                    data[i + 3] = force_data_VALUE.w; 
                }
            }
        }
    
        GLenum format = GL_RGBA;
    
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    void bind(GLenum textureUnit = GL_TEXTURE0) const {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    void unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint getID() const { return textureID; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    GLuint textureID;
    int width;
    int height;
    int channels;
};

#endif // TEXTURE_H