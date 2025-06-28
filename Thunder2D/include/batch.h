#pragma once
#include "fwd.hpp"
#include "max.h"
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <glad/gl.h>
#include <algorithm>
#include <t2dshader.h>
#define __2(_s,_n) (((_s) < (_n)) ? (_s) : (_n))

#undef near
#undef far

#include <iostream>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

class Camera2D {
private:
    glm::vec2 position = {0.0f, 0.0f};
    float zoom = 1.0f;
    float rotation = 0.0f;
    int viewportWidth = 800, viewportHeight = 600;

    mutable glm::mat4 viewMatrix = glm::mat4(1.0f);
    mutable glm::mat4 projectionMatrix = glm::mat4(1.0f);
    mutable glm::mat4 viewProjectionMatrix = glm::mat4(1.0f);
    mutable bool matricesDirty = true;

    void updateMatrices_noZoom() const{
        if (!matricesDirty) return;
        // View matrix = scale * rotation * translation
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-position, 0.0f));
        glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), -rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

        viewMatrix = scaleMat * rotationMat * translation;

        float halfWidth = viewportWidth * 0.5f;
        float halfHeight = viewportHeight * 0.5f;
        projectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);

        viewProjectionMatrix = projectionMatrix * viewMatrix;
        matricesDirty = false;
    }

    void updateMatrices() const {
        if (!matricesDirty) return;

        // View matrix = scale * rotation * translation
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-position, 0.0f));
        glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), -rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(zoom, zoom, 1.0f));

        viewMatrix = scaleMat * rotationMat * translation;

        float halfWidth = viewportWidth * 0.5f;
        float halfHeight = viewportHeight * 0.5f;
        projectionMatrix = glm::ortho(0.0f, (float)viewportWidth, 0.0f, (float)viewportHeight);

        viewProjectionMatrix = projectionMatrix * viewMatrix;
        matricesDirty = false;
    }

public:
    Camera2D(int width, int height)
        : viewportWidth(width), viewportHeight(height) {}

    void setPosition(const glm::vec2& pos) {
        position = pos;
        matricesDirty = true;
    }

    void setZoom(float z) {
        zoom = glm::clamp(z, 0.05f, 10.0f); // Clamp to safe range
        matricesDirty = true;
    }

    void setRotation(float rot) {
        rotation = rot;
        matricesDirty = true;
    }

    void setViewportSize(int width, int height) {
        viewportWidth = width;
        viewportHeight = height;
        matricesDirty = true;
    }

    void move(const glm::vec2& delta) {
        position += delta;
        matricesDirty = true;
    }

    void zoomBy(float factor) {
        zoom *= factor;
        zoom = glm::clamp(zoom, 0.05f, 10.0f);
        matricesDirty = true;
    }

    void rotate(float deltaRadians) {
        rotation += deltaRadians;
        matricesDirty = true;
    }

    const glm::mat4& getViewMatrix_noZoom(){
        updateMatrices_noZoom();
        return viewMatrix;
    }
    const glm::mat4& getViewProjectionMatrix_noZoom() const {
        updateMatrices_noZoom();
        return viewProjectionMatrix;
    }



    const glm::mat4& getViewMatrix() const {
        updateMatrices();
        return viewMatrix;
    }

    const glm::mat4& getProjectionMatrix() const {
        updateMatrices();
        return projectionMatrix;
    }

    const glm::mat4& getViewProjectionMatrix() const {
        updateMatrices();
        return viewProjectionMatrix;
    }

    glm::vec2 worldToScreen(const glm::vec2& worldPos) const {
        updateMatrices();
        glm::vec4 clip = viewProjectionMatrix * glm::vec4(worldPos, 0.0f, 1.0f);
        glm::vec2 ndc = glm::vec2(clip) / clip.w;
    
        float screenX = (ndc.x + 1.0f) * 0.5f * viewportWidth;
        float screenY = (ndc.y + 1.0f) * 0.5f * viewportHeight; 
    
        return glm::vec2(screenX, screenY);
    }

    glm::vec2 worldToScreen_YINVERTED(const glm::vec2& worldPos) const {
        updateMatrices();
        glm::vec4 clip = viewProjectionMatrix * glm::vec4(worldPos, 0.0f, 1.0f);
        glm::vec2 ndc = glm::vec2(clip) / clip.w;

        float screenX = (ndc.x + 1.0f) * 0.5f * viewportWidth;
        float screenY = (1.0f - ndc.y) * 0.5f * viewportHeight;

        return glm::vec2(screenX, screenY);
    }

    glm::vec2 screenToWorld(const glm::vec2& screenPos) const {
        updateMatrices();
    
        float x = (2.0f * screenPos.x) / viewportWidth - 1.0f;
        float y = (2.0f * screenPos.y) / viewportHeight - 1.0f; 
        glm::vec4 ndc = glm::vec4(x, y, 0.0f, 1.0f);
    
        glm::mat4 invVP = glm::inverse(viewProjectionMatrix);
        glm::vec4 world = invVP * ndc;
    
        return glm::vec2(world.x, world.y);
    }

    glm::vec2 screenToWorld_YINVERTED(const glm::vec2& screenPos) const {
        updateMatrices();

        float x = (2.0f * screenPos.x) / viewportWidth - 1.0f;
        float y = 1.0f - (2.0f * screenPos.y) / viewportHeight;
        glm::vec4 ndc = glm::vec4(x, y, 0.0f, 1.0f);

        glm::mat4 invVP = glm::inverse(viewProjectionMatrix);
        glm::vec4 world = invVP * ndc;

        return glm::vec2(world.x, world.y);
    }

    glm::vec2 getWorldBounds() const {
        float halfWidth = (viewportWidth * 0.5f) / zoom;
        float halfHeight = (viewportHeight * 0.5f) / zoom;
        return glm::vec2(halfWidth * 2.0f, halfHeight * 2.0f);
    }

    bool isPointVisible(const glm::vec2& point) const {
        glm::vec2 bounds = getWorldBounds();
        glm::vec2 halfBounds = bounds * 0.5f;

        return (
            point.x >= position.x - halfBounds.x &&
            point.x <= position.x + halfBounds.x &&
            point.y >= position.y - halfBounds.y &&
            point.y <= position.y + halfBounds.y
        );
    }

    void lookAt(const glm::vec2& target) {
        setPosition(target);
    }

    void centerOn(const glm::vec2& point) {
        setPosition(point);
    }

    glm::vec2 getPosition() const { return position; }
    float getZoom() const { return zoom; }
    float getRotation() const { return rotation; }
    glm::vec2 getViewportSize() const { return glm::vec2(viewportWidth, viewportHeight); }
};



class SpriteShader : public IShader {
private:
    GLuint ID;
    std::vector<GLuint>* textures; 
    
public:
    SpriteShader(const std::string& vertexPath, const std::string& fragmentPath, std::vector<GLuint>* textureArray) 
        : textures(textureArray) {
        init(vertexPath, fragmentPath);
    }
    
    ~SpriteShader() {
        glDeleteProgram(ID);
    }
    
    void use() const override {
        glUseProgram(ID);
    }
    
    void setProjection(int width, int height) override {
        float proj[16] = {
            2.0f / width, 0, 0, 0,
            0, -2.0f / height, 0, 0,
            0, 0, -1, 0,
            -1, 1, 0, 1
        };
        setUniform("projection", proj);
    }
    
    void setProjection(const float proj[16]) override {
        setUniform("projection", proj);
    }
    
    void setupUniforms() override {
        GLint texturesLoc = glGetUniformLocation(ID, "textures");
        if (texturesLoc != -1) {
            int samplerValues[8] = {0, 1, 2, 3, 4, 5, 6, 7};
            glUniform1iv(texturesLoc, 8, samplerValues);
        }
        
        if (textures) {
            for (int i = 0; i < __2((int)textures->size(), 8); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, (*textures)[i]);
            }
        }
    }
    
    GLuint getID() const override { return ID; }
    
private:
    void init(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode = loadFile(vertexPath);
        std::string fragmentCode = loadFile(fragmentPath);

        GLuint vertexShader = compileShader(vertexCode, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);

        GLint success;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(ID, 512, nullptr, infoLog);
            std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    void setUniform(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setUniform(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setUniform(const std::string& name, const float* matrix4) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, matrix4);
    }

    std::string loadFile(const std::string& path) const {
        std::ifstream file{path};
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    GLuint compileShader(const std::string& source, GLenum type) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
        }

        return shader;
    }
};


class LineShader : public IShader {
private:
    GLuint ID;
    
public:
    LineShader(const std::string& vertexPath, const std::string& fragmentPath) {
        init(vertexPath, fragmentPath);
    }
    
    ~LineShader() {
        glDeleteProgram(ID);
    }
    
    void use() const override {
        glUseProgram(ID);
    }
    
    void setProjection(int width, int height) override {
        float proj[16] = {
            2.0f / width, 0, 0, 0,
            0, -2.0f / height, 0, 0,
            0, 0, -1, 0,
            -1, 1, 0, 1
        };
        setUniform("projection", proj);
    }
    void setProjection(const float proj[16]) override {
        
        setUniform("projection", proj);
    }
    
    void setupUniforms() override {
    }
    
    GLuint getID() const override { return ID; }
    
private:
    void init(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode = loadFile(vertexPath);
        std::string fragmentCode = loadFile(fragmentPath);

        GLuint vertexShader = compileShader(vertexCode, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);

        GLint success;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(ID, 512, nullptr, infoLog);
            std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    void setUniform(const std::string& name, const float* matrix4) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, matrix4);
    }
    
    std::string loadFile(const std::string& path) const {
        std::ifstream file{path};
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    GLuint compileShader(const std::string& source, GLenum type) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
        }

        return shader;
    }
};
struct BaseVertex {
    virtual ~BaseVertex() = default;
    virtual size_t getSize() const = 0;
    virtual void* getData() = 0;
    virtual void setupAttributes() const = 0;
};

class IRenderable {
public:
    virtual ~IRenderable() = default;
    virtual bool isDirty() const = 0;
    virtual void setClean() = 0;
    virtual int getZOrder() const = 0;
    virtual int getTextureID() const = 0;
    virtual std::vector<std::unique_ptr<BaseVertex>> generateVertices() = 0;
    virtual std::vector<unsigned int> generateIndices(unsigned int vertexOffset) = 0;
    virtual int getVertexCount() const = 0;
    virtual int getIndexCount() const = 0;
    size_t vertexOffsetInBuffer = 0;
    size_t indexOffsetInBuffer = 0;
};

template<typename VertexType>
class AbstractBatchRenderer {
private:
std::vector<GLuint> textures;
std::unique_ptr<IShader> shader;

GLuint vao, vbo, ebo;
size_t indexCount = 0;
size_t maxVertices;
size_t maxIndices;

std::function<void()> setupVertexLayout;

public:
std::vector<std::shared_ptr<IRenderable>> renderables;
    AbstractBatchRenderer(size_t maxVerts, size_t maxIdxs, 
                         std::function<void()> vertexLayoutSetup,
                         std::unique_ptr<IShader> shaderPtr)
        : maxVertices(maxVerts), maxIndices(maxIdxs), 
          setupVertexLayout(vertexLayoutSetup), shader(std::move(shaderPtr)) {}
    
    void init() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(VertexType), nullptr, GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(unsigned int), nullptr, GL_STATIC_DRAW);
        
        if (setupVertexLayout) {
            setupVertexLayout();
        }
        
        glBindVertexArray(0);
    }
    
    void addRenderable(std::shared_ptr<IRenderable> renderable) {
        renderables.push_back(renderable);
    }
    
    void removeRenderable(std::shared_ptr<IRenderable> renderable) {
        auto it = std::find(renderables.begin(), renderables.end(), renderable);
        if (it != renderables.end()) {
            renderables.erase(it);
        }
    }
    
    void addTexture(GLuint texture) {
        textures.push_back(texture);
    }
    
    void setTextures(const std::vector<GLuint>& textureArray) {
        textures = textureArray;
    }
    
    void reload() {
        if (renderables.empty()) return;
        
        bool anyDirty = false;
        for (const auto& renderable : renderables) {
            if (renderable->isDirty()) {
                anyDirty = true;
                break;
            }
        }
        if (!anyDirty) return;
        

        
        std::sort(renderables.begin(), renderables.end(), 
            [](const auto& a, const auto& b) {
                return a->getZOrder() < b->getZOrder();
            });
        
        std::vector<VertexType> vertexData;
        std::vector<unsigned int> indexData;
        unsigned int vertexOffset = 0;
        unsigned int indexOffset = 0;
        
        for (const auto& renderable : renderables) {
            auto vertices = renderable->generateVertices();
            auto indices = renderable->generateIndices(vertexOffset);
            
            renderable->vertexOffsetInBuffer = vertexOffset;
            renderable->indexOffsetInBuffer = indexOffset;

            for (const auto& vertex : vertices) {
                vertexData.push_back(*static_cast<const VertexType*>(vertex->getData()));
            }
            
            indexData.insert(indexData.end(), indices.begin(), indices.end());
            
            vertexOffset += static_cast<unsigned int>(renderable->getVertexCount());
            indexOffset += static_cast<unsigned int>(indices.size());
            renderable->setClean();
        }
        


        //! 
        //std::cout << "Vertices: " << vertexData.size() << ", Indices: " << indexData.size() << "\n";

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(VertexType), vertexData.data(), GL_DYNAMIC_DRAW);
     
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof(unsigned int), indexData.data(), GL_DYNAMIC_DRAW);

        indexCount = indexData.size();
    }
    void updateDirtyRenderables() {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    
        for (auto& renderable : renderables) {
            if (!renderable->isDirty()) continue;
    
            auto vertices = renderable->generateVertices();
            auto indices = renderable->generateIndices(renderable->vertexOffsetInBuffer);
    
            std::vector<VertexType> tempVertexData;
            for (const auto& vertex : vertices) {
                tempVertexData.push_back(*static_cast<const VertexType*>(vertex->getData()));
            }
    
            glBufferSubData(GL_ARRAY_BUFFER,
                            renderable->vertexOffsetInBuffer * sizeof(VertexType),
                            tempVertexData.size() * sizeof(VertexType),
                            tempVertexData.data());
    
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                            renderable->indexOffsetInBuffer * sizeof(unsigned int),
                            indices.size() * sizeof(unsigned int),
                            indices.data());
    

            renderable->setClean();
        }
    }
    void render(const float proj[16]) {
        if (renderables.empty() || !shader) return;
        
        shader->use();
        shader->setProjection(proj);
        shader->setupUniforms();
        
        glBindVertexArray(vao);
        
        if (indexCount > 0) {
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        }
        
        cleanup();
    }
    
    void destroy() {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    }
    
private:
    void cleanup() {
        glBindVertexArray(0);
        glUseProgram(0);
        
        for (int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE0);
    }
};

struct SpriteVertex : public BaseVertex {
    struct Data {
        float x, y, u, v;
        float r,g,b,a;
        int textureID;
    } data;
    
    SpriteVertex(float x, float y, float u, float v,float r,float g,float b,float a, int texID) 
        : data{x, y, u, v,r,g,b,a,texID} {}
    
    size_t getSize() const override { return sizeof(Data); }
    void* getData() override { return &data; }
    
    void setupAttributes() const override {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 2));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 4));
        glEnableVertexAttribArray(2);
        
        glVertexAttribIPointer(3, 1, GL_INT, sizeof(Data), (void*)(sizeof(float) * 8));
        glEnableVertexAttribArray(3);
    }
};

struct LineVertex : public BaseVertex {
    struct Data {
        float x, y;
        float r, g, b, a;
        float thickness;
        float type;
        float localx,localy;
    } data;
    
    LineVertex(float x, float y, float r, float g, float b, float a, float thickness,float type,float localx,float localy)
        : data{x, y, r, g, b, a, thickness,type,localx,localy} {}
    
    size_t getSize() const override { return sizeof(Data); }
    void* getData() override { return &data; }
    
    void setupAttributes() const override {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 2));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 6));
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 7));
        glEnableVertexAttribArray(3);

        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 8));
        glEnableVertexAttribArray(4);
    }
};

class Sprite : public IRenderable {
private:
    
    int zOrder = 0;
    int textureID = 0;
    bool dirty = true;
    
    public:
    max::vec2<float> pos, scale;
    max::vec4<float> color;
    float rotation = 0.0f;
    std::array<max::vec2<float>, 4> txCoords;
    Sprite(max::vec2<float> position, max::vec2<float> size, int texID,max::vec4<float> color = {1,1,1,1})
        : pos(position), scale(size), textureID(texID),color(color) {
        txCoords = {
            max::vec2<float>{0.0f, 0.0f},  // Bottom-left
            max::vec2<float>{1.0f, 0.0f},  // Bottom-right
            max::vec2<float>{1.0f, 1.0f},  // Top-right
            max::vec2<float>{0.0f, 1.0f}   // Top-left
        };
    }
    
    void dirt(){dirty= true;}
    void txid(int txid){textureID = txid;}
    bool isDirty() const override { return dirty; }
    void setClean() override { dirty = false; }
    int getZOrder() const override { return zOrder; }
    int getTextureID() const override { return textureID; }
    int getVertexCount() const override { return 4; }
    int getIndexCount() const override { return 6; }
    
    std::vector<std::unique_ptr<BaseVertex>> generateVertices() override {
        max::vec2<float> halfScale = scale * 0.5f;
    
        max::vec2<float> corners[4] = {
            {-halfScale.x, halfScale.y}, 
            { halfScale.x, halfScale.y}, 
            { halfScale.x,  -halfScale.y}, 
            {-halfScale.x,  -halfScale.y}  
        };
    
        if (rotation != 0.0f) {
            for (int i = 0; i < 4; ++i) {
                max::rotate(corners[i], rotation);
            }
        }
    
        for (int i = 0; i < 4; ++i) {
            corners[i] += pos;
        }
    
        std::vector<std::unique_ptr<BaseVertex>> vertices;
        for (int i = 0; i < 4; ++i) {
            vertices.push_back(std::make_unique<SpriteVertex>(
                corners[i].x, corners[i].y,
                txCoords[i].x, txCoords[i].y,
                color.x,color.y,color.z,color.w,
                textureID
            ));
        }
    
        return vertices;
    }
    
    std::vector<unsigned int> generateIndices(unsigned int vertexOffset) override {
        unsigned int offset = static_cast<unsigned int>(vertexOffset);
        return {
            offset + 0u, offset + 1u, offset + 2u,
            offset + 2u, offset + 3u, offset + 0u
        };
    }
    
    void setPosition(max::vec2<float> position) { pos = position; dirty = true; }
    void setScale(max::vec2<float> size) { scale = size; dirty = true; }
    void setRotation(float rot) { rotation = rot; dirty = true; }
    void setZOrder(int z) { zOrder = z; dirty = true; }
};

class Line : public IRenderable {
public:
    max::vec2<float> start, end;
    max::vec4<float> color;
    float thickness;
    int zOrder = 0;
    bool dirty = true;

    Line(max::vec2<float> startPos, max::vec2<float> endPos, max::vec4<float> col, float thick)
        : start(startPos), end(endPos), color(col), thickness(thick) {}

    void dirt(){dirty = true;}
    bool isDirty() const override { return dirty; }
    void setClean() override { dirty = false; }
    int getZOrder() const override { return zOrder; }
    int getTextureID() const override { return 0; } 
    int getVertexCount() const override { return 4; }
    int getIndexCount() const override { return 6; }
    
    std::vector<std::unique_ptr<BaseVertex>> generateVertices() override {
        max::vec2<float> dir = end - start;
        max::vec2<float> perp = max::vec2<float>(-dir.y, dir.x);
        max::normalize(perp);
        perp *= (thickness * 0.5f);
        const float uvs[4][2] = {
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f}
        };
        std::vector<std::unique_ptr<BaseVertex>> vertices;
        vertices.push_back(std::make_unique<LineVertex>(
            start.x - perp.x, start.y - perp.y, color.x, color.y, color.z, color.w, thickness,0.0f,uvs[0][0],uvs[0][1]));
        vertices.push_back(std::make_unique<LineVertex>(
            start.x + perp.x, start.y + perp.y, color.x, color.y, color.z, color.w, thickness,0.0f,uvs[1][0],uvs[1][1]));
        vertices.push_back(std::make_unique<LineVertex>(
            end.x + perp.x, end.y + perp.y, color.x, color.y, color.z, color.w, thickness,0.0f,uvs[2][0],uvs[2][1]));
        vertices.push_back(std::make_unique<LineVertex>(
            end.x - perp.x, end.y - perp.y, color.x, color.y, color.z, color.w, thickness,0.0f,uvs[3][0],uvs[3][1]));
        
        return vertices;
    }
    
    std::vector<unsigned int> generateIndices(unsigned int vertexOffset) override {
        unsigned int offset = static_cast<unsigned int>(vertexOffset);
        return {
            offset + 0u, offset + 1u, offset + 2u,
            offset + 2u, offset + 3u, offset + 0u
        };
    }
};


class Point : public IRenderable {
public:
    max::vec2<float> position;
    max::vec4<float> color;
    float size;
    int zOrder = 0;
    bool dirty = true;

    Point(max::vec2<float> pos, max::vec4<float> col, float s)
        : position(pos), color(col), size(s) {}

    void dirt(){dirty = true;}
    bool isDirty() const override { return dirty; }
    void setClean() override { dirty = false; }
    int getZOrder() const override { return zOrder; }
    int getTextureID() const override { return 0; }
    int getVertexCount() const override { return 4; }
    int getIndexCount() const override { return 6; }

    std::vector<std::unique_ptr<BaseVertex>> generateVertices() override {
        float halfSize = size * 0.5f;


        std::vector<std::unique_ptr<BaseVertex>> vertices;
        vertices.push_back(std::make_unique<LineVertex>(
            position.x - halfSize, position.y - halfSize, color.x, color.y, color.z, color.w, size,1.0f,0.0f,0.0f));
        vertices.push_back(std::make_unique<LineVertex>(
            position.x + halfSize, position.y - halfSize, color.x, color.y, color.z, color.w, size,1.0f,1.0f,0.0f));
        vertices.push_back(std::make_unique<LineVertex>(
            position.x + halfSize, position.y + halfSize, color.x, color.y, color.z, color.w, size,1.0f,1.0f,1.0f));
        vertices.push_back(std::make_unique<LineVertex>(
            position.x - halfSize, position.y + halfSize, color.x, color.y, color.z, color.w, size,1.0f,0.0f,1.0f));

        return vertices;
    }

    std::vector<unsigned int> generateIndices(unsigned int vertexOffset) override {
        unsigned int offset = static_cast<unsigned int>(vertexOffset);
        return {
            offset + 0u, offset + 1u, offset + 2u,
            offset + 2u, offset + 3u, offset + 0u
        };
    }
};



class Box2D_DebugRenderer{
    public:
    std::shared_ptr<AbstractBatchRenderer<LineVertex::Data>> renderer;
    std::unique_ptr<IShader> shader;
    std::vector<std::shared_ptr<Line>> lines;
    std::vector<std::shared_ptr<Point>> points;

    int locLines = 0;
    int locPoints = 0;

    Box2D_DebugRenderer(){
        shader = ShaderFactory::create<LineShader>("resources/shaders/debug.vert","resources/shaders/debug.frag");

        renderer = std::make_shared<AbstractBatchRenderer<LineVertex::Data>>(
            500, 500*(3/2), 
            []() { 
                LineVertex dummy{0,0,0,0,0,0,0,0,0,0};
                dummy.setupAttributes(); 
            },
            std::move(shader)
        );
        renderer->init();
    }
    
    int addline(max::vec2<float> start,max::vec2<float> end,max::vec4<float> color,float thickness){
        auto line = std::make_shared<Line>(start,end,color,thickness);
        renderer->addRenderable(line);
        lines.push_back(line);
        return locLines++;
    }
    int addPoint(max::vec2<float> pos,max::vec4<float> color = {1,0,0,1},float radius = 4.0f){
        auto point = std::make_shared<Point>(pos,color,radius);
        renderer->addRenderable(point);
        points.push_back(point);
        return locPoints++;
    }



    std::shared_ptr<Line> line(int loc){
        return lines[loc];
    }
    void line_set(int loc,max::vec2<float> start,max::vec2<float> end){
        lines[loc]->start = start;
        lines[loc]->end = end;
    }
    void dirt_line(int loc){
        lines[loc]->dirt();
    }
    bool removeLine(int loc){
        renderer->removeRenderable(lines[loc]);
        return true;
    }

    std::shared_ptr<Point> point(int loc){
        return points[loc];
    }
    void point_pos_set(int loc,max::vec2<float> pos){
        points[loc]->position = pos;
    }
    void dirt_point(int loc){
        points[loc]->dirt();
    }
    bool removePoint(int loc){
        renderer->removeRenderable(points[loc]);
        return true;
    }

   
    void render(const float *proj){
        renderer->reload();
        renderer->render(proj);
    }


    
};