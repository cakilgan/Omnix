#ifndef T_2DSHADER_H
#define T_2DSHADER_H
#include <glad/gl.h>
#include <memory>

class IShader {
public:
    virtual ~IShader() = default;
    virtual void use() const = 0;
    virtual void setProjection(int width, int height) = 0;
    virtual void setProjection(const float proj[16]) = 0;
    virtual void setupUniforms() = 0;
    virtual GLuint getID() const = 0;
};

class ShaderFactory {
public:
    template<typename ShaderType, typename... Args>
    static std::unique_ptr<IShader> create(Args&&... args) {
        return std::make_unique<ShaderType>(std::forward<Args>(args)...);
    }
};

#endif // T_2DSHADER_H