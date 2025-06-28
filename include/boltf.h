#ifndef BOLTF_H
#define BOLTF_H

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>
#include <max.h>
#include <glad/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

struct Character {
    GLuint textureID;   
    max::vec2<int> size;
    max::vec2<int> bearing;   
    GLuint advance;     
};

class BoltF {
private:
    std::unordered_map<char, Character> characters;
    FT_Library ft;
    FT_Face face;
    
    GLuint VAO, VBO;
    GLuint shaderProgram;
    GLuint vertexShader, fragmentShader;
    
    GLint projectionLoc;
    GLint textColorLoc;
    GLint textureLoc;
    
    max::vec2<int> screenSize;
    
    
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 vertex;
        layout (location = 1) in vec2 texCoord;
        
        out vec2 TexCoords;
        
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * vec4(vertex, 0.0, 1.0);
            TexCoords = texCoord;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        
        uniform sampler2D text;
        uniform vec3 textColor;
        
        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        }
    )";
    
    bool initialized = false;
    bool fontLoaded = false;

public:
    BoltF() : VAO(0), VBO(0), shaderProgram(0), vertexShader(0), fragmentShader(0) {
        if (FT_Init_FreeType(&ft)) {
            std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        }
    }

    static BoltF& getInstance(){
        static BoltF fr;
        return fr;
    }
    
    
    ~BoltF() {
        std::cout<<"deconstruct"<<std::endl;
        cleanup();
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }
    
    void cleanup() {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (shaderProgram != 0) {
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }
        if (vertexShader != 0) {
            glDeleteShader(vertexShader);
            vertexShader = 0;
        }
        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
        }
        
        for (auto& pair : characters) {
            glDeleteTextures(1, &pair.second.textureID);
        }
        characters.clear();
        
        initialized = false;
        fontLoaded = false;
    }
    
    bool checkShaderCompile(GLuint shader, const std::string& type) {
        GLint success;
        GLchar infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
        return true;
    }
    
    bool checkProgramLink(GLuint program) {
        GLint success;
        GLchar infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
            return false;
        }
        return true;
    }
    
    bool initShaders() {
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        if (!checkShaderCompile(vertexShader, "VERTEX")) return false;
        
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        if (!checkShaderCompile(fragmentShader, "FRAGMENT")) return false;
        
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        if (!checkProgramLink(shaderProgram)) return false;
        
        projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        textColorLoc = glGetUniformLocation(shaderProgram, "textColor");
        textureLoc = glGetUniformLocation(shaderProgram, "text");
        
        return true;
    }
    
    void init(const std::string& fontPath, int fontSize) {
        if (initialized) {
            cleanup();
        }
        
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cerr << "ERROR::FREETYPE: Failed to load font: " << fontPath << std::endl;
            return;
        }
        
        FT_Set_Pixel_Sizes(face, 0, fontSize);
        
        if (!initShaders()) {
            std::cerr << "Failed to initialize shaders" << std::endl;
            return;
        }
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cerr << "ERROR::FREETYPE: Failed to load Glyph for char: " << (int)c << std::endl;
                continue;
            }
            
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            Character character = {
                texture,
                max::vec2<int>(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                max::vec2<int>(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<GLuint>(face->glyph->advance.x)
            };
            characters.insert(std::pair<char, Character>(c, character));
        }
        
        initialized = true;
        fontLoaded = true;
        
        std::cout << "BoltF initialized successfully with font: " << fontPath << std::endl;
    }
    
    void begin(const max::vec2<int>& screenSize) {
        if (!initialized || !fontLoaded) {
            std::cerr << "BoltF not initialized!" << std::endl;
            return;
        }
        
        this->screenSize = screenSize;
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glUseProgram(shaderProgram);
        
        float projection[16] = {
            2.0f / screenSize.x, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / screenSize.y, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 1.0f
        };
        
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
        glUniform1i(textureLoc, 0);
        
        glUniform3f(textColorLoc, 1.0f, 1.0f, 1.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);
    }
    
    std::vector<std::pair<Character, float[4][6]>> totalVertex;
    void draw(const std::string& text, const max::vec2<float>& scale, const max::vec2<float>& location) {
        if (!initialized || !fontLoaded) {
            return;
        }
        
        float x = location.x;
        float y = location.y;
        
        std::string::const_iterator c;
        for (c = text.begin(); c != text.end(); c++) {
            Character ch = characters[*c];
            
            float xpos = x + ch.bearing.x * scale.x;
            float ypos = y - (ch.size.y - ch.bearing.y) * scale.y;
            
            float w = ch.size.x * scale.x;
            float h = ch.size.y * scale.y;
            
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };
            
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            x += (ch.advance >> 6) * scale.x;
        }
    }
    
    void end() {
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glDisable(GL_BLEND);
    }
    
    void setTextColor(float r, float g, float b) {
        if (initialized) {
            glUseProgram(shaderProgram);
            glUniform3f(textColorLoc, r, g, b);
        }
    }
    
    max::vec2<float> getTextSize(const std::string& text, const max::vec2<float>& scale) {
        if (!fontLoaded) return max::vec2<float>(0, 0);
        
        float width = 0;
        float height = 0;
        
        for (char c : text) {
            Character ch = characters[c];
            width += (ch.advance >> 6) * scale.x;
            #define tssmax(a,b)            (((a) > (b)) ? (a) : (b))
            height = tssmax(height, ch.size.y * scale.y);
        }
        
        return max::vec2<float>(width, height);
    }
    
    bool isInitialized() const { return initialized && fontLoaded; }
};

#define INIT_BF(fontpath,size)\
BoltF::getInstance().init(fontpath,size);\

#define BEGINBF(screenscalex,screenscaley)\
BoltF::getInstance().begin(max::vec2<int>{screenscalex,screenscaley});\

#define ENDBF()\
BoltF::getInstance().end();\

#define TEXTBF(text,scalex,scaley,locx,locy)\
BoltF::getInstance().draw(text,max::vec2<float>(scalex,scaley),max::vec2<float>(locx,locy));\

#define COLORBF(colorr,colorg,colorb)\
BoltF::getInstance().setTextColor(colorr,colorg,colorb);\


struct __Vec2 {
    float x, y;
    __Vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
};

struct __Vec2i {
    int x, y;
    __Vec2i(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Color {
    float r, g, b, a;
    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

class TextureAtlas {
private:
    GLuint textureID;
    int width, height;
    
public:
    TextureAtlas() : textureID(0), width(0), height(0) {}
    
    ~TextureAtlas() {
        if (textureID) {
            glDeleteTextures(1, &textureID);
        }
    }
    
    bool loadFromFile(const std::string& filepath) {
        // Bu kısımda gerçek uygulamada SOIL, stb_image veya benzeri bir kütüphane kullanılır
        // Şimdilik placeholder texture oluşturalım
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // 256x256 beyaz texture (test için)
        width = 256;
        height = 256;
        std::vector<unsigned char> data(width * height * 4, 255);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }
    
    void bind() const {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
    
    void unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

class TextureFontRenderer {
private:
    // Screen buffer boyutları
    int screenWidth, screenHeight;
    
    // Karakter grid boyutları
    int cellWidth, cellHeight;
    
    // Texture atlas bilgileri
    int atlasGridWidth, atlasGridHeight; // Atlas'taki grid boyutu (örn: 16x16)
    __Vec2i atlasCellSize; // Her karakterin pixel boyutu
    
    // Screen buffer - her hücre bir karakter
    std::vector<std::vector<unsigned char>> screenBuffer;
    
    // OpenGL nesneleri
    GLuint VAO, VBO;
    GLuint shaderProgram;
    GLuint vertexShader, fragmentShader;
    
    // Shader uniform konumları
    GLint projectionLoc, textColorLoc, textureLoc;
    
    // Vertex verileri
    std::vector<float> vertices;
    
    // Texture atlas
    TextureAtlas atlas;
    
    // Karakter pattern string'i (ASCII sıralı)
    std::string characterPattern;
    
    // UV koordinat haritası
    std::map<unsigned char, __Vec2> uvCoords;
    
    // Shader kodları
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec2 texCoord;
        
        out vec2 TexCoords;
        
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
            TexCoords = texCoord;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 FragColor;
        
        uniform sampler2D fontTexture;
        uniform vec3 textColor;
        
        void main() {
            vec4 sampled = texture(fontTexture, TexCoords);
            FragColor = vec4(textColor, 1.0) * sampled;
        }
    )";
    
    bool compileShader(GLuint shader, const std::string& type) {
        GLint success;
        GLchar infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
        return true;
    }
    
    bool linkProgram(GLuint program) {
        GLint success;
        GLchar infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
            return false;
        }
        return true;
    }
    
    bool initializeShaders() {
        // Vertex shader
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        if (!compileShader(vertexShader, "VERTEX")) return false;
        
        // Fragment shader
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        if (!compileShader(fragmentShader, "FRAGMENT")) return false;
        
        // Shader program
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        if (!linkProgram(shaderProgram)) return false;
        
        // Uniform konumları
        projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        textColorLoc = glGetUniformLocation(shaderProgram, "textColor");
        textureLoc = glGetUniformLocation(shaderProgram, "fontTexture");
        
        return true;
    }
    
    void generateUVCoordinates() {
        uvCoords.clear();
        
        float cellW = 1.0f / atlasGridWidth;
        float cellH = 1.0f / atlasGridHeight;
        
        for (size_t i = 0; i < characterPattern.size(); ++i) {
            int col = i % atlasGridWidth;
            int row = i / atlasGridWidth;
            
            float u = col * cellW;
            float v = 1.0f - (row + 1) * cellH; // Y koordinatını ters çevir
            
            unsigned char ch = static_cast<unsigned char>(characterPattern[i]);
            uvCoords[ch] = __Vec2(u, v);
        }
    }
    
    void rebuildVertexData() {
        vertices.clear();
        
        float cellW = 1.0f / atlasGridWidth;
        float cellH = 1.0f / atlasGridHeight;
        
        for (int row = 0; row < screenHeight; ++row) {
            for (int col = 0; col < screenWidth; ++col) {
                unsigned char ch = screenBuffer[row][col];
                
                // Boş karakterleri atla
                if (ch == 0 || ch == ' ') continue;
                
                // UV koordinatlarını al
                auto it = uvCoords.find(ch);
                if (it == uvCoords.end()) continue;
                
                __Vec2 uv = it->second;
                
                // Screen pozisyonları (pixel cinsinden)
                float xPos = col * cellWidth;
                float yPos = row * cellHeight;
                
                // İki üçgen oluştur (quad)
                float quad[6][4] = {
                    // İlk üçgen
                    { xPos,              yPos + cellHeight, uv.x,        uv.y },
                    { xPos,              yPos,              uv.x,        uv.y + cellH },
                    { xPos + cellWidth,  yPos,              uv.x + cellW, uv.y + cellH },
                    
                    // İkinci üçgen
                    { xPos,              yPos + cellHeight, uv.x,        uv.y },
                    { xPos + cellWidth,  yPos,              uv.x + cellW, uv.y + cellH },
                    { xPos + cellWidth,  yPos + cellHeight, uv.x + cellW, uv.y }
                };
                
                // Vertex verilerini ekle
                for (int i = 0; i < 6; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        vertices.push_back(quad[i][j]);
                    }
                }
            }
        }
    }
    
public:
    TextureFontRenderer() : 
        screenWidth(0), screenHeight(0),
        cellWidth(8), cellHeight(16),
        atlasGridWidth(16), atlasGridHeight(16),
        atlasCellSize(16, 16),
        VAO(0), VBO(0), shaderProgram(0),
        vertexShader(0), fragmentShader(0) {
        
        // ASCII karakterleri için standart pattern
        characterPattern = " !\"#$%&'()*+,-./0123456789:;<=>?"
                          "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                          "`abcdefghijklmnopqrstuvwxyz{|}~";
    }
    
    ~TextureFontRenderer() {
        cleanup();
    }
    
    bool initialize(const std::string& atlasPath, 
                   int screenW, int screenH,
                   int charW, int charH,
                   int atlasGridW = 16, int atlasGridH = 16,
                   const std::string& pattern = "") {
        
        screenWidth = screenW;
        screenHeight = screenH;
        cellWidth = charW;
        cellHeight = charH;
        atlasGridWidth = atlasGridW;
        atlasGridHeight = atlasGridH;
        
        if (!pattern.empty()) {
            characterPattern = pattern;
        }
        
        // Screen buffer'ı initialize et
        screenBuffer.resize(screenHeight, std::vector<unsigned char>(screenWidth, ' '));
        
        // Texture atlas'ı yükle
        if (!atlas.loadFromFile(atlasPath)) {
            std::cerr << "Failed to load texture atlas: " << atlasPath << std::endl;
            return false;
        }
        
        // Shader'ları initialize et
        if (!initializeShaders()) {
            std::cerr << "Failed to initialize shaders" << std::endl;
            return false;
        }
        
        // OpenGL bufferları oluştur
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        
        // Dinamik buffer oluştur
        glBufferData(GL_ARRAY_BUFFER, screenWidth * screenHeight * 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
        
        // Vertex attribute'ları ayarla
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        // UV koordinatlarını oluştur
        generateUVCoordinates();
        
        return true;
    }
    
    void setCharacter(int x, int y, unsigned char ch) {
        if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight) {
            screenBuffer[y][x] = ch;
        }
    }
    
    void setText(int x, int y, const std::string& text) {
        for (size_t i = 0; i < text.length() && (x + i) < screenWidth; ++i) {
            setCharacter(x + i, y, text[i]);
        }
    }
    
    void clearScreen() {
        for (auto& row : screenBuffer) {
            std::fill(row.begin(), row.end(), ' ');
        }
    }
    
    void updateBuffer() {
        rebuildVertexData();
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    void render(const Color& textColor = Color(1.0f, 1.0f, 1.0f, 1.0f)) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glUseProgram(shaderProgram);
        
        // Projection matrix (orthographic)
        float totalWidth = screenWidth * cellWidth;
        float totalHeight = screenHeight * cellHeight;
        
        float projection[16] = {
            2.0f / totalWidth, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / totalHeight, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 1.0f
        };
        
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
        glUniform3f(textColorLoc, textColor.r, textColor.g, textColor.b);
        glUniform1i(textureLoc, 0);
        
        atlas.bind();
        glBindVertexArray(VAO);
        
        int vertexCount = vertices.size() / 4;
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        
        glBindVertexArray(0);
        atlas.unbind();
        glUseProgram(0);
        glDisable(GL_BLEND);
    }
    
    void cleanup() {
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (shaderProgram) {
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }
        if (vertexShader) {
            glDeleteShader(vertexShader);
            vertexShader = 0;
        }
        if (fragmentShader) {
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
        }
    }
    
    // Utility fonksiyonlar
    int getScreenWidth() const { return screenWidth; }
    int getScreenHeight() const { return screenHeight; }
    int getCellWidth() const { return cellWidth; }
    int getCellHeight() const { return cellHeight; }
};
#endif // BOLTF_H