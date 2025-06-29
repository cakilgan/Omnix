#ifndef THUNDER_2D_H
#define THUNDER_2D_H

#include "BoltID.h"
#include "OmnixModules.h"
#include "batch.h"
#include "boltlog.h"
#include "Omnix.h"
#include "box2d.h"
#include "freetype/freetype.h"
#include "glad/wgl.h"
#include "gtc/type_ptr.hpp"
#include "id.h"
#include "max.h"
#include "t2dshader.h"
#include "test_utils.h"
#include "types.h"
#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <vcruntime_new.h>
#include <vector>
#include <texture.h>
#include <string>
#include <batch.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#undef far
#undef near
#define NOMINMAX

std::vector<std::array<max::vec2<float>, 4>> parse_sheet(max::vec2<float> texture_size,max::vec2<float> cell_size,bool mirrorX = false);

struct instant_animation{
    std::vector<std::array<max::vec2<float>, 4>> anim_txcoords;
    std::map<std::string, std::vector<int>> animations;
    std::string currentAnimation; 
    int currentFrame; 
    float animationTime;
    float frameDuration;

    void update(float dt,std::shared_ptr<Sprite>& sprite){
         if (currentAnimation.empty() || anim_txcoords.empty()) return;

        animationTime += dt;

        if (animationTime >= frameDuration) {
            animationTime -= frameDuration;
            currentFrame++;
            const auto& frames = animations[currentAnimation];
            if (currentFrame >= static_cast<int>(frames.size())) {
                currentFrame = 0; 
            }
            sprite->txCoords = anim_txcoords[frames[currentFrame]];
            sprite->dirt();
        }
    }
    void switchto(const std::string& name,std::shared_ptr<Sprite>& sprite){
        if (name != currentAnimation && animations.count(name)) {
            currentAnimation = name;
            currentFrame = 0;
            animationTime = 0.0f;
            sprite->txCoords = anim_txcoords[animations[currentAnimation][currentFrame]];
            sprite->dirt();
        }
    }
};

enum MapObjectState{
    SOLID = 1,
    EMPTY = 0,
};

struct MapObject{
    int mapLoc;
    max::vec2<float> pos;
    max::vec2<float> scale;
    int txLoc = -1;
    std::shared_ptr<Sprite> spritePtr;
    MapObjectState state = EMPTY;
    bool dirty = true;

    b2BodyId bodid = b2_nullBodyId;
    bool havePhysics(){
        return b2Body_IsValid(bodid);
    }
};

template <size_t xCount, size_t yCount>
class Map{
public:
    MapObject objs[xCount][yCount];
    
    inline void parse(max::vec2<float> startPos, max::vec2<float> mapScale, max::vec2<float> objectScalePattern){
        max::vec2<float> oneScale = mapScale / objectScalePattern;
        int mapLoc = 0;
        max::vec2<float> offset = objectScalePattern * 0.5f;
    
        for (int i = 0; i < oneScale.y; i++) {
            for (int j = 0; j < oneScale.x; j++) {
                auto newPos = startPos + max::vec2<float>{j * objectScalePattern.x, i * objectScalePattern.y} + offset;
                MapObject mapobj{mapLoc, newPos, objectScalePattern, -1, std::make_shared<Sprite>(newPos, objectScalePattern, -1)};
                objs[j][i] = mapobj;
                mapLoc++;
            }
        }
    }
    
    static b2BodyId create_physics_optimized(Map& map, b2WorldId worldId, float pixelsPerMeter = 32.0f){
        std::vector<b2Vec2> edgePoints;
        
        for (int y = 0; y < yCount; y++) {
            for (int x = 0; x < xCount; x++) {
                if (map.objs[x][y].state == SOLID) {
                    addTileEdges(map, x, y, edgePoints, pixelsPerMeter);
                }
            }
        }
        
        if (edgePoints.empty()) {
            return b2_nullBodyId;
        }
        
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
        
        b2ChainDef chainDef = b2DefaultChainDef();
        chainDef.points = edgePoints.data();
        chainDef.count = edgePoints.size();
        chainDef.isLoop = true;
        
        
        b2CreateChain(bodyId, &chainDef);
        
        return bodyId;
    }
    
    static std::vector<b2BodyId> create_physics_merged(Map& map, b2WorldId worldId, std::vector<std::vector<std::array<int, 2>>>& result ,float pixelsPerMeter = 32.0f){
        std::vector<b2BodyId> bodies;
        
        std::vector<TileRect> mergedRects = mergeAdjacentTiles(map,result);
        
        for (const auto& rect : mergedRects) {
            b2BodyId bodyId = createRectangleBody(map, rect, worldId, pixelsPerMeter);
            if (b2Body_IsValid(bodyId)) {
                bodies.push_back(bodyId);
            }
        }
        
        return bodies;
    }
    
    void state(int x, int y, MapObjectState newState) {
        if (x >= 0 && x < xCount && y >= 0 && y < yCount) {
            objs[x][y].state = newState;
        }
    }
    void m_state(int y,MapObjectState newState){
        for (int i = 0; i < xCount; i++)
        {
            state(i, y, newState);
        }
    }
    void txID(int x,int y, int id){
        if (x >= 0 && x < xCount && y >= 0 && y < yCount) {
            objs[x][y].txLoc = id;
        }
    }
    void m_state(int y,int id){
        for (int i = 0; i < xCount; i++)
        {
            txID(i, y, id);
        }
    }
    void color(int x,int y,max::vec4<float> _color){
        if (x >= 0 && x < xCount && y >= 0 && y < yCount) 
         objs[x][y].spritePtr->color = _color;
    }
    void m_color(int y,max::vec4<float> _color){
        for (int i = 0; i < xCount; i++)
        {
            color(i, y, _color);
        }
    }
    

    void refresh(){
        for (int y = 0; y < yCount; y++) {
            for (int x = 0; x < xCount; x++) {
                if(objs[x][y].dirty){
                    objs[x][y].spritePtr->txid(objs[x][y].txLoc);
                    objs[x][y].spritePtr->pos = {objs[x][y].pos};
                    objs[x][y].spritePtr->scale = {objs[x][y].scale};
                    objs[x][y].spritePtr->dirt();
                    objs[x][y].dirty = false;
                }
            }
        }
    }
    void dirt_all(){
        for (int y = 0; y < yCount; y++) {
            for (int x = 0; x < xCount; x++) {
                objs[x][y].dirty = true;
            }
        }
    }
    void dirt(int x,int y){
        objs[x][y].dirty = true;
    }


    
    MapObjectState get_state(int x, int y) const {
        if (x >= 0 && x < xCount && y >= 0 && y < yCount) {
            return objs[x][y].state;
        }
        return EMPTY;
    }
    
    static inline Map initialize_and_create_map(std::unique_ptr<AbstractBatchRenderer<SpriteVertex::Data>> &renderer, 
                                              max::vec2<float> startPos, 
                                              max::vec2<float> mapScale, 
                                              max::vec2<float> objectScalePattern){
        Map map{};
        map.parse(startPos, mapScale, objectScalePattern);
        
        for(int y = 0; y < yCount; y++){
            for(int x = 0; x < xCount; x++){
                renderer->addRenderable(map.objs[x][y].spritePtr);
            }
        }
        
        return map;
    }
    static inline Map* initialize_and_create_map_HEAP(std::unique_ptr<AbstractBatchRenderer<SpriteVertex::Data>> &renderer, 
                                              max::vec2<float> startPos, 
                                              max::vec2<float> mapScale, 
                                              max::vec2<float> objectScalePattern){
        Map *map = new Map();
        map->parse(startPos, mapScale, objectScalePattern);
        
        for(int y = 0; y < yCount; y++){
            for(int x = 0; x < xCount; x++){
                renderer->addRenderable(map->objs[x][y].spritePtr);
            }
        }
        
        return map;
    }

private:
    struct TileRect {
        int x, y, width, height;
        max::vec2<float> center;
        max::vec2<float> size;
    };
    
    static void addTileEdges(Map& map, int tileX, int tileY, std::vector<b2Vec2>& edges, float pixelsPerMeter = 32.0f) {
        const MapObject& tile = map.objs[tileX][tileY];
        max::vec2<float> pos = tile.pos;
        max::vec2<float> scale = tile.scale;
        
        bool hasTopNeighbor = (tileY > 0 && map.objs[tileX][tileY-1].state == SOLID);
        bool hasBottomNeighbor = (tileY < yCount-1 && map.objs[tileX][tileY+1].state == SOLID);
        bool hasLeftNeighbor = (tileX > 0 && map.objs[tileX-1][tileY].state == SOLID);
        bool hasRightNeighbor = (tileX < xCount-1 && map.objs[tileX+1][tileY].state == SOLID);
        
        float left = pos.x / pixelsPerMeter;
        float right = (pos.x + scale.x) / pixelsPerMeter;
        float bottom = pos.y / pixelsPerMeter;
        float top = (pos.y + scale.y) / pixelsPerMeter;
        
        if (!hasTopNeighbor) {
            edges.push_back(b2Vec2(left, top));
            edges.push_back(b2Vec2(right, top));
        }
        
        if (!hasRightNeighbor) {
            edges.push_back(b2Vec2(right, top));
            edges.push_back(b2Vec2(right, bottom));
        }
        
        if (!hasBottomNeighbor) {
            edges.push_back(b2Vec2(right, bottom));
            edges.push_back(b2Vec2(left, bottom));
        }
        
        if (!hasLeftNeighbor) {
            edges.push_back(b2Vec2(left, bottom));
            edges.push_back(b2Vec2(left, top));
        }
    }
    
    static std::vector<TileRect> mergeAdjacentTiles(Map& map,std::vector<std::vector<std::array<int, 2>>>& result) {
        std::vector<TileRect> rects;
        bool processed[xCount][yCount] = {false};
        
        for (int y = 0; y < yCount; y++) {
            for (int x = 0; x < xCount; x++) {
                if (map.objs[x][y].state == SOLID && !processed[x][y]) {
                    std::vector<std::array<int, 2>> __arr;
                    TileRect rect = findLargestRect(map, x, y, processed,__arr);
                    rects.push_back(rect);
                    result.push_back(__arr);
                }
            }
        }
        
        return rects;
    }
    
    static TileRect findLargestRect(Map& map, int startX, int startY, bool processed[xCount][yCount],std::vector<std::array<int, 2>>& bodies) {
        int width = 0;
        while (startX + width < xCount &&
               map.objs[startX + width][startY].state == SOLID &&
               !processed[startX + width][startY]) {
            width++;
        }
    
        int height = 1;
        bool canExpand = true;
        while (canExpand && startY + height < yCount) {
            for (int x = startX; x < startX + width; x++) {
                if (map.objs[x][startY + height].state != SOLID ||
                    processed[x][startY + height]) {
                    canExpand = false;
                    break;
                }
            }
            if (canExpand) height++;
        }
    
        
        for (int y = startY; y < startY + height; y++) {
            for (int x = startX; x < startX + width; x++) {
                bodies.push_back({x,y});
                processed[x][y] = true;
            }
        }
    
        max::vec2<float> tileScale = map.objs[startX][startY].scale;
        float centerX = map.objs[startX][startY].pos.x + (width - 1) * tileScale.x * 0.5f;
        float centerY = map.objs[startX][startY].pos.y + (height - 1) * tileScale.y * 0.5f;
    
        max::vec2<float> center = { centerX, centerY };
        max::vec2<float> size = { width * tileScale.x, height * tileScale.y };
    
        return { startX, startY, width, height, center, size };
    }
    static b2BodyId createRectangleBody(Map& map, const TileRect& rect, b2WorldId worldId, float pixelsPerMeter = 32.0f) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
    
        bodyDef.position = {rect.center.x/pixelsPerMeter,rect.center.y/pixelsPerMeter};
    
        b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
        b2Polygon box = b2MakeBox((rect.size.x/2.0f)/pixelsPerMeter, (rect.size.y/2.0f)/pixelsPerMeter);
    
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.material.friction = 0.3f;
    
        b2CreatePolygonShape(bodyId, &shapeDef, &box);
        return bodyId;
    }
};

struct SceneObject{
    std::shared_ptr<Sprite> sprite;
    b2BodyId body = b2_nullBodyId;

    std::function<void(SceneObject& self)> updatefnc;

    max::vec2<float> sprite_body_offset = {0.0f,0.0f};

    std::map<std::string,float> infos;

    bool sync_body_and_sprite = true;

    bool have_physics(){
        return b2Body_IsValid(body);
    }
    void set_sync_body_and_sprite(bool v0){
        sync_body_and_sprite = v0;
    }
    void set_sprite_body_offset(max::vec2<float> offset){
        sprite_body_offset = offset;
    }
};

class Scene{
    public:
    max::vec2<int> screenSize{600,600};
    std::vector<SceneObject> objects;
    int loc = 0;
    float pixels_per_meter = 64.0f;


    std::vector<GLuint>* spriteTextures;
    Camera2D& sceneCam;
    Scene(Camera2D& cam):sceneCam(cam){}
    b2WorldId physics_world;
    std::unique_ptr<IShader> spriteBatchShader;
    std::unique_ptr<AbstractBatchRenderer<SpriteVertex::Data>> spriteBatch;


    void set_pixels_per_meter(float pixels_per_meter){
        this->pixels_per_meter = pixels_per_meter;
    }
    

    int add_no_physics_object(max::vec2<float> pos,max::vec2<float> size,int txLoc = -1){
        auto sprite = std::make_shared<Sprite>(pos, size, txLoc);
        spriteBatch->addRenderable(sprite); 
        return add_object(sprite,b2_nullBodyId);
    }
    int add_object(max::vec2<float> pos,max::vec2<float> size,int txLoc = -1,float friction = 0.3f,float density = 1.0f){
        auto sprite = std::make_shared<Sprite>(pos, size, txLoc);
        spriteBatch->addRenderable(sprite); 
        return add_object(sprite,max::physics::create_dynamic_box(physics_world,size/2.0f,pos,pixels_per_meter,friction,density));
    }
    int add_object_static(max::vec2<float> pos,max::vec2<float> size,int txLoc = -1){
        auto sprite = std::make_shared<Sprite>(pos, size, txLoc);
        spriteBatch->addRenderable(sprite); 
        return add_object(sprite,max::physics::create_static_box(physics_world,size/2.0f,pos,pixels_per_meter));
    }
    int add_object_dynamic(max::vec2<float> pos,max::vec2<float> size,int txLoc = -1){
        auto sprite = std::make_shared<Sprite>(pos, size, txLoc);
        spriteBatch->addRenderable(sprite); 
        return add_object(sprite,max::physics::create_dynamic_box(physics_world,size/2.0f,pos,pixels_per_meter));
    }
    int add_object(std::shared_ptr<Sprite> sprite,b2BodyId bodid){
        objects.push_back({sprite,bodid,nullptr});
        return loc++;
    }
    int add_object(SceneObject obj){
        objects.push_back(obj);
        return loc++;
    }

    void init(max::vec2<float> gravity = {0.0f,-10.0f},std::vector<GLuint> __spriteTextures = {}){
        this->spriteTextures = new std::vector<GLuint>{__spriteTextures};

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {gravity.x,gravity.y};
        physics_world = b2CreateWorld(&worldDef);

        sceneCam = {screenSize.x,screenSize.y};

        spriteBatchShader = ShaderFactory::create<SpriteShader>(
                    "resources/shaders/sprite.vert", "resources/shaders/sprite.frag", spriteTextures);

        spriteBatch = std::make_unique<AbstractBatchRenderer<SpriteVertex::Data>>(
            0,0, 
            []() { 
                SpriteVertex dummy(0,0,0,0,0,0,0,0,0);
                dummy.setupAttributes(); 
            },
            std::move(spriteBatchShader)
        );

        spriteBatch->init();
        spriteBatch->setTextures(*spriteTextures);
        spriteBatch->reload();
    }
    void update_cam(const max::vec2<int>& screenSize){
        this->screenSize = screenSize;

        sceneCam.setViewportSize(screenSize.x, screenSize.y);
    }
    void update(float dt){
        
        for(auto &obj:objects){
            if(obj.have_physics()&&obj.sync_body_and_sprite){
                auto pos = b2Body_GetPosition(obj.body);
                if(pos.x*pixels_per_meter+obj.sprite_body_offset.x==obj.sprite->pos.x&&pos.y*pixels_per_meter+obj.sprite_body_offset.y==obj.sprite->pos.y){
                    continue;
                }
                obj.sprite->pos = (max::vec2<float>{pos.x,pos.y})*pixels_per_meter+obj.sprite_body_offset;
                auto rot = atan2(b2Body_GetRotation(obj.body).s,b2Body_GetRotation(obj.body).c);
                obj.sprite->setRotation(rot);
                std::cout<<"ROT:: "<<obj.sprite->rotation<<std::endl;
                obj.sprite->dirt();
            }
           
        }

        b2World_Step(physics_world, dt, 4);

        spriteBatch->updateDirtyRenderables();
        spriteBatch->render(glm::value_ptr(sceneCam.getViewProjectionMatrix()));
    }

    template<size_t xCount,size_t yCount> void init_map(Map<xCount,yCount>* map){
        std::vector<std::vector<std::array<int, 2>>> res;
        auto res_bodies = map->create_physics_merged(*map, this->physics_world, res,pixels_per_meter);
        for (int y = 0; y < yCount; ++y) {
                    for (int x = 0; x < xCount; ++x) {
                        auto& obj = map->objs[x][y];
                        
                        if (!obj.havePhysics()) {
                            for (int vector_index = 0; vector_index < res.size(); ++vector_index) {
                                const auto& coord_vector = res[vector_index];
                                
                                for (const auto& coord : coord_vector) {
                                    if (coord[0] == x && coord[1] == y) {
                                        if (vector_index < res_bodies.size()) {
                                            obj.bodid = res_bodies[vector_index]; 
                                        }
                                        break; 
                                    }
                                }
                            }
                            SceneObject _obj;
                            _obj.sprite = obj.spritePtr;
                            _obj.body = obj.bodid;
                            _obj.infos["x"] = (float)x;
                            _obj.infos["y"] = (float)y;
                            _obj.sync_body_and_sprite = false; 
                            add_object(_obj);
                        }
                        
                        if (obj.havePhysics()) {
                            SceneObject _obj;
                            _obj.sprite = obj.spritePtr;
                            _obj.body = obj.bodid;
                            _obj.infos["x"] = (float)x;
                            _obj.infos["y"] = (float)y;
                            _obj.sync_body_and_sprite = true; 
                            
                            _obj.updatefnc = [this](SceneObject& self){
                                float grid_x = self.infos.at("x");
                                float grid_y = self.infos.at("y");
                                
                                float tile_width = self.sprite->scale.x;
                                float tile_height = self.sprite->scale.y;
                                
                                float target_sprite_x = grid_x * tile_width+(std::abs(tile_width)/2);
                                float target_sprite_y = grid_y * tile_height+(std::abs(tile_height)/2);
                                
                                b2Vec2 body_pos = b2Body_GetPosition(self.body);
                                float body_world_x = body_pos.x * pixels_per_meter;
                                float body_world_y = body_pos.y * pixels_per_meter;
                                
                                float offset_x = target_sprite_x - body_world_x;
                                float offset_y = target_sprite_y - body_world_y;
                                
                                self.set_sprite_body_offset({offset_x, offset_y});
                            };
                            _obj.updatefnc(_obj);
                            
                            add_object(_obj);
                        }
                    }
                }
    }

    void destroy(){
        b2DestroyWorld(physics_world);
        delete this->spriteTextures;
    }
};





#define OMNIX_UI_FONT 0
#define OMNIX_UI_BUTTON 1

namespace t2d::ui{
    struct UIVertex:BaseVertex{
      struct Data {
          float x, y;
          float u, v;
          float r,g,b,a;
          int txID;
          int type;
      } data;
      UIVertex(float x, float y,float u,float v, float r, float g, float b, float a,int txID,int type)
          : data{x,y,u,v,r,g,b,a,txID,type} {}
      
      size_t getSize() const override { return sizeof(Data); }
      void* getData() override { return &data; }
      
      void setupAttributes() const override {
              glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)0);
              glEnableVertexAttribArray(0);
              
              glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 2));
              glEnableVertexAttribArray(1);
              
              glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)(sizeof(float) * 4));
              glEnableVertexAttribArray(2);
      
              glVertexAttribIPointer(3, 1, GL_INT, sizeof(Data), (void*)((sizeof(float) * 8)));
              glEnableVertexAttribArray(3);
      
              glVertexAttribIPointer(4, 1, GL_INT, sizeof(Data), (void*)((sizeof(float) * 8)+sizeof(int)));
              glEnableVertexAttribArray(4);
          }
    
    };
    struct Character {
        GLuint textureID;   
        max::vec2<int> size;
        max::vec2<int> bearing;   
        GLuint advance;     
        max::vec2<float> uvMin;
        max::vec2<float> uvMax;
    };

    class UIGlyph : public IRenderable {
        public:
        max::vec2<float> pos, scale;
        max::vec4<float> color{0,0,0,0};
        char character;
        bool dirty = true;
        int txLoc = 1;
        bool visible = false;
        std::unordered_map<char, Character> chars;
        UIGlyph(std::unordered_map<char, Character> chars,char ch, max::vec2<float> position, max::vec2<float> size)
            : chars(chars),character(ch), pos(position), scale(size) {


            }
        UIGlyph(){}

        std::vector<std::unique_ptr<BaseVertex>> generateVertices() override {
            const auto& ch = chars[character];

            max::vec2<float> half = scale * 0.5f;
            max::vec2<float> corners[4] = {
                {-half.x, -half.y},
                { half.x, -half.y},
                { half.x,  half.y},
                {-half.x,  half.y}
            };
        
            for (int i = 0; i < 4; ++i)
                corners[i] += pos;

            std::vector<std::unique_ptr<BaseVertex>> vertices;
            

            if(!visible){
            //! check
            vertices.push_back(std::make_unique<UIVertex>(corners[0].x, corners[0].y, ch.uvMin.x, ch.uvMin.y, color.x,color.y,color.z,0, txLoc, OMNIX_UI_FONT));
            vertices.push_back(std::make_unique<UIVertex>(corners[1].x, corners[1].y, ch.uvMax.x, ch.uvMin.y, color.x,color.y,color.z,0, txLoc, OMNIX_UI_FONT));
            vertices.push_back(std::make_unique<UIVertex>(corners[2].x, corners[2].y, ch.uvMax.x, ch.uvMax.y, color.x,color.y,color.z,0, txLoc, OMNIX_UI_FONT));
            vertices.push_back(std::make_unique<UIVertex>(corners[3].x, corners[3].y, ch.uvMin.x, ch.uvMax.y, color.x,color.y,color.z,0, txLoc, OMNIX_UI_FONT));
            }else{
            vertices.push_back(std::make_unique<UIVertex>(corners[0].x, corners[0].y, ch.uvMin.x, ch.uvMin.y, color.x,color.y,color.z,color.w, txLoc, OMNIX_UI_FONT));
            vertices.push_back(std::make_unique<UIVertex>(corners[1].x, corners[1].y, ch.uvMax.x, ch.uvMin.y, color.x,color.y,color.z,color.w, txLoc, OMNIX_UI_FONT));
            vertices.push_back(std::make_unique<UIVertex>(corners[2].x, corners[2].y, ch.uvMax.x, ch.uvMax.y, color.x,color.y,color.z,color.w, txLoc, OMNIX_UI_FONT));
            vertices.push_back(std::make_unique<UIVertex>(corners[3].x, corners[3].y, ch.uvMin.x, ch.uvMax.y, color.x,color.y,color.z,color.w, txLoc, OMNIX_UI_FONT));
            }
            
            return vertices;
        }

        std::vector<unsigned int> generateIndices(unsigned int offset) override {
            return {
                offset + 0, offset + 1, offset + 2,
                offset + 2, offset + 3, offset + 0
            };
        }

        int getVertexCount() const override { return 4; }
        int getIndexCount() const override { return 6; }
        bool isDirty() const override { return dirty; }
        void setClean() override { dirty = false; }
        int getZOrder() const override { return 0; }
        int getTextureID() const override { return 0; }
    };

    static int frameDirtCalls = 0;
    class E_UIQuadBase : public IRenderable {
    private:
        
        int zOrder = 0;
        int textureID = 0;
        bool dirty = true;
        
        public:
        max::vec2<float> pos, scale;
        max::vec4<float> color;
        float rotation = 0.0f;
        std::array<max::vec2<float>, 4> txCoords;
        E_UIQuadBase(max::vec2<float> position, max::vec2<float> size,max::vec4<float> color, int texID,std::array<max::vec2<float>, 4> txCoords = {
                max::vec2<float>{0.0f, 0.0f},  // Bottom-left
                max::vec2<float>{1.0f, 0.0f},  // Bottom-right
                max::vec2<float>{1.0f, 1.0f},  // Top-right
                max::vec2<float>{0.0f, 1.0f}   // Top-left
        })
            : pos(position), scale(size),color(color), textureID(texID),txCoords(txCoords) {
        }
        
        
        void dirt(){
            dirty= true;
            frameDirtCalls++;
        }
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
                {-halfScale.x, -halfScale.y}, 
                { halfScale.x, -halfScale.y}, 
                { halfScale.x,  halfScale.y}, 
                {-halfScale.x,  halfScale.y}  
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

                vertices.push_back(std::make_unique<UIVertex>(
                    corners[i].x, corners[i].y,
                    txCoords[i].x, txCoords[i].y,
                    color.x,color.y,color.z,color.w,
                    textureID,OMNIX_UI_BUTTON
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

    
    class UIFont {
        public:
        std::unordered_map<char, Character> chars;
        GLuint fontAtlasTexture = 0;
        static inline FT_Library ft;
        static inline FT_Face face;

        static std::unordered_map<char, Character> init(const std::string& fontPath, int fontSize,GLuint *fontAtlasTexture) {
            std::unordered_map<char, Character> characters;

            if (FT_Init_FreeType(&ft)) {
                std::cerr << "FREETYPE: Init failed" << std::endl;
                return {};
            }
            if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
                std::cerr << "FREETYPE: Font load failed" << std::endl;
                return {};
            }
            FT_Set_Pixel_Sizes(face, 0, fontSize);

            const int atlasW = 512, atlasH = 512;
            int x = 0, y = 0, rowHeight = 0;
            std::vector<unsigned char> atlas(atlasW * atlasH, 0);

            for (unsigned char c = 0; c < 128; ++c) {
                if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
                if (x + face->glyph->bitmap.width >= atlasW) {
                    x = 0;
                    y += rowHeight;
                    rowHeight = 0;
                }

                for (int i = 0; i < face->glyph->bitmap.rows; ++i) {
                    for (int j = 0; j < face->glyph->bitmap.width; ++j) {
                        atlas[(y + i) * atlasW + (x + j)] = face->glyph->bitmap.buffer[(face->glyph->bitmap.rows - 1 - i) * face->glyph->bitmap.width + j];
                    }
                }

                characters[c] = {
                    *fontAtlasTexture,
                    max::vec2<int>(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    max::vec2<int>(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    static_cast<GLuint>(face->glyph->advance.x),
                    max::vec2<float>((float)x / atlasW, (float)y / atlasH),
                    max::vec2<float>((float)(x + face->glyph->bitmap.width) / atlasW, (float)(y + face->glyph->bitmap.rows) / atlasH)
                };

                x += face->glyph->bitmap.width + 1;
                #define __max(a,b) (((a) > (b)) ? (a) : (b))
                rowHeight = __max(rowHeight, face->glyph->bitmap.rows);
            }

            glGenTextures(1, fontAtlasTexture);
            glBindTexture(GL_TEXTURE_2D, *fontAtlasTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasW, atlasH, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            return characters;
        }
    };


    class UIShader : public IShader {
private:
    GLuint ID;
    std::vector<GLint>* textures;

public:
    UIShader(const std::string& vertexPath, const std::string& fragmentPath, std::vector<GLint>* textureArray)
        : textures(textureArray) {
        init(vertexPath, fragmentPath);
    }

    ~UIShader() {
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
            int samplerValues[32] = {0, 1, 2, 3, 4, 5, 6, 7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
            glUniform1iv(texturesLoc, 32, samplerValues);
        }
        
        if (textures) {
            for (int i = 0; i < __2((int)textures->size(), 32); i++) {
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


    struct UIElement{
        UIManager* manager = nullptr;
        std::vector<UIElement*> childs;
        UIElement* parent;
        max::vec2<float> position;
        max::vec2<float> size;
        max::vec4<float> color;
        bool isVisible = true;
        bool canInteract = true;

        UIElement(max::vec2<float> position,max::vec2<float> size,max::vec4<float> color = {1,1,1,1}):position(position),size(size),color(color){}

        inline void add(UIElement* element){
           element->parent = this;
           childs.push_back(element);
        }
        virtual void draw(UIRenderer* renderer) = 0;
        virtual void update(UIRenderer* uirenderer) = 0;

        void dispose(){
            for(auto child:childs){
                delete child;
            }
        }

        virtual ~UIElement() = default;
    };


    struct GlyphPool {
        std::vector<std::shared_ptr<UIGlyph>> pool;
        size_t cursor = 0;
    
        void reset() {
            cursor = 0;
        }
    
        std::shared_ptr<UIGlyph> getGlyph(std::unique_ptr<AbstractBatchRenderer<UIVertex::Data>>& renderer) {
            if (cursor >= pool.size()) {
                std::cout<<"new"<<std::endl;
                pool.push_back(std::make_shared<UIGlyph>());
                renderer->addRenderable(pool[cursor]);
                renderer->reload();
                return pool[cursor++];
            }
            return pool[cursor++];
        }
    
        void finalizeFrame() {
            for (size_t i = cursor; i < pool.size(); ++i) {
                if (pool[i]->visible) {
                    pool[i]->visible = false;
                    pool[i]->dirty = true;  
                }
            }
            pool.back()->scale = {0,0};
        }
    };
    class UIRenderer {
       GlyphPool pool{};
       public: 
        std::unique_ptr<AbstractBatchRenderer<UIVertex::Data>> renderer;
        std::unique_ptr<IShader> shader;
        max::vec2<float> screenSize;
        
        
        std::vector<UIFont*> fonts{};

        UIRenderer(max::vec2<float> screenSize,std::vector<GLint>& textures)
            : screenSize(screenSize) {

            shader = std::make_unique<UIShader>(
                "resources/shaders/ui.vert",
                "resources/shaders/ui.frag",
                &textures
            );

            renderer = std::make_unique<AbstractBatchRenderer<UIVertex::Data>>(
                10000/4, (10000/4)*3/2,
                []() {
                    UIVertex dummy(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    dummy.setupAttributes();
                },
                std::move(shader)
            );

            renderer->init();

            
        }

        void refresh(){
            renderer->reload();
        }
        void begin() {
            pool.reset();
        }

         void drawText(const std::string& text, max::vec2<float> position, max::vec2<float> scale,max::vec4<float> color={1,1,1,1}, int fontLoc = 0, int fonttxLoc = 0) {
            float cursorX = position.x;
            for (int i = 0; i < text.size(); i++) {
                auto it = fonts[fontLoc]->chars.find(text[i]);
                if (it == fonts[fontLoc]->chars.end()) continue; 
        
                const Character& ch = it->second;
        
                float xpos = cursorX + ch.bearing.x * scale.x;
                float ypos = position.y - (ch.size.y - ch.bearing.y);
        
                float w = ch.size.x * scale.x;
                float h = ch.size.y * scale.y;
        

                auto glyph = pool.getGlyph(renderer);
                if(
                glyph->pos == max::vec2<float>{xpos + w / 2.0f, ypos + h / 2.0f}
                &&glyph->scale == max::vec2<float>{w, h}
                &&glyph->character == text[i]
                &&max::equalsv4(glyph->color, color)
                &&glyph->txLoc == fonttxLoc
                ){
                    cursorX += (ch.advance >> 6) * scale.x;
                    continue;
                }else{
                    glyph->chars = fonts[fontLoc]->chars;
                    glyph->pos = max::vec2<float>{xpos + w / 2.0f, ypos + h / 2.0f};
                    glyph->scale = max::vec2<float>{w, h};
                    glyph->character = text[i];
                   
                    glyph->color = color;
                    glyph->txLoc = fonttxLoc;
    
                    glyph->dirty = true;
                }
                
                glyph->visible = true;
        
                cursorX += (ch.advance >> 6) * scale.x;

            }
        }
        
        void drawElement(std::shared_ptr<UIElement> element){
            element->draw(this);
        }
        void drawElement(UIElement* element){
            element->draw(this);
        }

        std::shared_ptr<UIGlyph> renderable_GLYPH(int index){
            return std::dynamic_pointer_cast<UIGlyph>(renderer->renderables[index]);
        }
        
        int onetimereload = 1;
        void end(Camera2D *viewPortCam) {
            renderer->updateDirtyRenderables();

            float projection[16] = {
                2.0f / viewPortCam->getViewportSize().x, 0.0f, 0.0f, 0.0f,
                0.0f, 2.0f / viewPortCam->getViewportSize().y, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f,
               -1.0f, -1.0f, 0.0f, 1.0f
            };

            renderer->render(projection);

            pool.finalizeFrame();
        }

        void dispose(){
            for(auto font:fonts){
                delete font;
            }
        }

    };
    struct UIButton:public UIElement{
        std::string buttonText;
        max::vec2<float> buttonTextPos;
        max::vec2<float> buttonTextScaleModifier;

        int txLoc = -1;
        UIButton(max::vec2<float> pos,max::vec2<float> scale,max::vec4<float> color):UIElement(pos,scale,color){}
        void draw(UIRenderer* renderer){
            renderer->renderer->addRenderable(std::make_shared<E_UIQuadBase>(position,size,color,txLoc));
        };
        virtual void update(UIRenderer* uirenderer) = 0;
    };


    class UIManager{
        public:
        std::vector<UIElement*> elements;

        Omnix::Core::Omnix& omnix;
        UIManager(Omnix::Core::Omnix& omnix):omnix(omnix){}
        void publish_event(Omnix::Defaults::OmnixUIEvent* event){
            omnix.eventBus().publish(event);
        }

        void draw(UIRenderer* renderer){
            for(auto element:elements){
                renderer->drawElement(element);
            }
        }

        void update(UIRenderer* uirenderer){
            for(auto element:elements){
                element->update(uirenderer);
            }
        }

        void dispose(){
            for(auto element:elements){
                element->dispose();
                delete element;
            }
        }
    };

    

    // empty = 0 , hovered = 1 , clicked 2
    struct DefaultButton:public UIButton{
        private:
        BoltID id;
        max::vec4<float> __cl_holder;
        int __tx_holder = 0;
        std::array<max::vec2<float>, 4> __txc_holder;
        int last = 0;
        public:
        std::shared_ptr<E_UIQuadBase> renderable;
        std::function<void(DefaultButton* self,UIRenderer* uirenderer)> updateFnc;
        BoltID get_id(){
            return id;
        }
        max::vec4<float> hoverColor {1,0,1,1};
        max::vec4<float> clickColor {0,1,0,1};

        int hoverTxID = 0;
        int clickTxID = 0;

        std::array<max::vec2<float>, 4> hoverTXCoords;
        std::array<max::vec2<float>, 4> clickTXCoords;

        std::array<max::vec2<float>, 4> txCoords = {
                max::vec2<float>{0.0f, 0.0f},  
                max::vec2<float>{1.0f, 0.0f},  
                max::vec2<float>{1.0f, 1.0f},  
                max::vec2<float>{0.0f, 1.0f}   
        };

        Camera2D *cam;
        DefaultButton(max::vec2<float> pos,max::vec2<float> scale,max::vec4<float> color):t2d::ui::UIButton(pos,scale,color){
            id = BoltID::randomBoltID(1);
        }
        
        void init(){
            __cl_holder = color;
            __tx_holder = this->txLoc;
            __txc_holder = this->txCoords;
        }
        void draw(t2d::ui::UIRenderer* uirenderer) override{
            if(!isVisible) return;
            renderable = std::make_shared<t2d::ui::E_UIQuadBase>(position,size,color,txLoc,txCoords);
            uirenderer->renderer->addRenderable(renderable);
        };
        void update(UIRenderer* uirenderer) override{
            if(!canInteract) return;


            int mousex = Omnix::Helpers::np_get_data<int,IDataProvider::__variants>("OmnixMouseModule", {OMNIX_MOUSE_POS_X});
            int mousey = Omnix::Helpers::np_get_data<int,IDataProvider::__variants>("OmnixMouseModule", {OMNIX_MOUSE_POS_Y,OMNIX_INVERTED});
    
            
            bool check = max::math::inside_region(mousex,mousey, position.x-(size/2).x, position.y+(size/2).y, position.x+(size/2).x, position.y-(size/2).y);
            if(check){
                bool isClicked = Omnix::Helpers::np_get_data<bool,IDataProvider::__variants>("OmnixMouseModule", {OMNIX_JUST_PRESS,OMNIX_MOUSE_LEFT_BUTTON});
                bool holding = Omnix::Helpers::np_get_data<bool,IDataProvider::__variants>("OmnixMouseModule", {OMNIX_PRESS,OMNIX_MOUSE_LEFT_BUTTON});
                if(isClicked){
                    color = clickColor;
                    this->txLoc = clickTxID;
                    this->txCoords = clickTXCoords;
                    auto __id = get_id();
                    auto __ouie = Omnix::Defaults::OmnixUIEvent{OMNIX_UI_ELEMENT,OMNIX_UI_ELEMENT_BUTTON,OMNIX_UI_ELEMENT_BUTTON_CLICK,__id};
                    manager->publish_event(&__ouie);
                    if(last!=2){
                        renderable->dirt();
                    }
                    last = 2;
                }
                else if(holding){
                    color = clickColor;
                    this->txLoc = clickTxID;
                    this->txCoords = clickTXCoords;
                    auto __id = get_id();
                    auto __ouie = Omnix::Defaults::OmnixUIEvent{OMNIX_UI_ELEMENT,OMNIX_UI_ELEMENT_BUTTON,OMNIX_UI_ELEMENT_BUTTON_HOLD,__id};
                    manager->publish_event(&__ouie);
                    if(last!=2){
                        renderable->dirt();
                    }
                    last = 2;
                }else{
                    color = hoverColor;
                    this->txLoc = hoverTxID;
                    this->txCoords = hoverTXCoords;
                    if(last!=1){
                        renderable->dirt();
                    }
                    last = 1;
                }
            }else{
                color = __cl_holder;
                this->txLoc = __tx_holder;
                this->txCoords = __txc_holder;
                auto __id = get_id();
                auto __ouie = Omnix::Defaults::OmnixUIEvent{OMNIX_UI_ELEMENT,OMNIX_UI_ELEMENT_BUTTON,OMNIX_UI_ELEMENT_BUTTON_RELEASE,__id};
                manager->publish_event(&__ouie);
                if(last!=0){
                    renderable->dirt();
                }
                last = 0;
            }


            renderable->pos = position;
            renderable->scale = size;
            renderable->txCoords = txCoords;
            renderable->txid(txLoc);
            renderable->color = color;


            if(updateFnc){
                updateFnc(this,uirenderer);
            }

            
        }
    };

    struct UILayout{
        virtual void apply(std::vector<UIElement*>& children,const max::vec2<float>& center,const max::vec2<float>& frameSize) = 0;
        virtual ~UILayout() = default;
    };
    struct VerticalLayout : public UILayout {
        float spacing = 4.0f;
    
        VerticalLayout(float spacing = 4.0f) : spacing(spacing) {}
    
        void apply(std::vector<UIElement*>& children, const max::vec2<float>& framePos, const max::vec2<float>& frameSize) override {
            if (children.empty()) return;
    
            float totalHeight = 0.0f;
            for (auto& child : children)
                totalHeight += child->size.y;
            totalHeight += spacing * (children.size() - 1);
    
            float startY = framePos.y + frameSize.y / 2.0f - totalHeight / 2.0f;
    
            for (auto& child : children) {
                float height = child->size.y;
                child->position = (max::vec2<float>{framePos.x - child->size.x / 2.0f, startY});
                startY += height + spacing;
            }
        }
    };
    struct HorizontalLayout : public UILayout {
        float spacing = 4.0f;
    
        HorizontalLayout(float spacing = 4.0f) : spacing(spacing) {}
    
        void apply(std::vector<UIElement*>& children, const max::vec2<float>& framePos, const max::vec2<float>& frameSize) override {
            if (children.empty()) return;
    
            float totalWidth = 0.0f;
            for (auto& child : children)
                totalWidth += child->size.x;
            totalWidth += spacing * (children.size() - 1);
    
            float startX = framePos.x - frameSize.x / 2.0f + (frameSize.x - totalWidth) / 2.0f;
    
            for (auto& child : children) {
            float width = child->size.x;
            child->position = max::vec2<float>{startX + width / 2.0f, framePos.y};
            startX += width + spacing;
            }
        }
    };

    struct UIFrame:public UIElement{
        UILayout* layout;
        std::function<void(UIFrame* self)> updateFnc;
        std::shared_ptr<E_UIQuadBase> renderable;
        int txLoc = -1;
        UIFrame(max::vec2<float> position,max::vec2<float> size,max::vec4<float> color = {1,1,1,1}):UIElement(position, size,color){}

        void draw(UIRenderer* renderer) override{
            renderable = std::make_shared<E_UIQuadBase>(position,size,color,txLoc);
            renderer->renderer->addRenderable(renderable);
            for(auto child:childs){
                child->draw(renderer);
            }
        }
        void update(UIRenderer* uirenderer) override{
            if(updateFnc)
             updateFnc(this);

            renderable->pos = position;
            renderable->scale = size;
            
            layout->apply(childs, position,size);
            for(auto child:childs){
                child->update(uirenderer);
            }
        }
        void dispose(){
            UIElement::dispose();
            delete layout;
        }
    };

    

    struct UIAdjusterButton:public DefaultButton{
        UIAdjusterButton(max::vec2<float> pos,max::vec2<float> scale,max::vec4<float> color):DefaultButton(pos, scale, color){};
        UIFrame *left,*right;
        bool dragging = false;
        int lastMouseX = 0;

        void init(){
            DefaultButton::init();
            OMNIX_EVENT(Omnix::Defaults::OmnixUIEvent, UiEvent){
                if (event->eventType == OMNIX_UI_ELEMENT && event->elementType == OMNIX_UI_ELEMENT_BUTTON) {
                    if (event->id == get_id()) {
                        if (event->action == OMNIX_UI_ELEMENT_BUTTON_CLICK) {
                            dragging = true;
                            lastMouseX = Omnix::Helpers::np_get_data<int, IDataProvider::__variants>("OmnixMouseModule", {OMNIX_MOUSE_POS_X});
                        }
                    }
                }
            };
            
            manager->omnix.eventBus().subscribe(UiEvent);
        };

    };


    static inline DefaultButton* newButton(
        max::vec2<float> startPos,
        max::vec2<float> scale,
        int txLoc,
        int clickTxLoc,
        int hoverTxLoc,
        max::vec4<float> color,
        max::vec4<float> hoverColor,
        max::vec4<float> clickColor,
        std::array<max::vec2<float>, 4> txCoords,
        std::array<max::vec2<float>, 4> clickTxCoords,
        std::array<max::vec2<float>, 4> hoverTxCoords,
        std::string buttonText,
        max::vec2<float> buttonTextScaleModifier,
        Camera2D* cam,
        UIManager* manager,
        std::function<void(DefaultButton* self,UIRenderer* uirenderer)> updateFnc = [](t2d::ui::DefaultButton* self,t2d::ui::UIRenderer* uirenderer){
                        if(self->buttonTextPos!=self->position - max::vec2<float>{static_cast<float>(self->buttonText.size()*(8*self->buttonTextScaleModifier.x)),8*self->buttonTextScaleModifier.y}){
                            self->buttonTextPos = self->position - max::vec2<float>{static_cast<float>(self->buttonText.size()*(8*self->buttonTextScaleModifier.x)),8*self->buttonTextScaleModifier.y};
                            self->renderable->dirt();
                        }
                        uirenderer->drawText(self->buttonText, self->buttonTextPos, self->buttonTextScaleModifier);
                    }
    ){
        DefaultButton* button = new DefaultButton(startPos,scale,color);
                
        button->txLoc = txLoc;
        button->clickTxID = clickTxLoc;
        button->hoverTxID = hoverTxLoc;

        button->hoverColor = hoverColor;
        button->clickColor = clickColor;

        button->txCoords  = txCoords;
        button->clickTXCoords = clickTxCoords;
        button->hoverTXCoords = hoverTxCoords;

        button->buttonText = buttonText;
        
        button->buttonTextScaleModifier = buttonTextScaleModifier;

        button->cam = cam;

        button->manager = manager;

        button->updateFnc = updateFnc;

        button->init();
        return button;
    }

    static inline UIAdjusterButton* newAdjusterButton(
        max::vec2<float> startPos,
        max::vec2<float> scale,
        int txLoc,
        int clickTxLoc,
        int hoverTxLoc,
        max::vec4<float> color,
        max::vec4<float> hoverColor,
        max::vec4<float> clickColor,
        std::array<max::vec2<float>, 4> txCoords,
        std::array<max::vec2<float>, 4> clickTxCoords,
        std::array<max::vec2<float>, 4> hoverTxCoords,
        std::string buttonText,
        max::vec2<float> buttonTextScaleModifier,
        Camera2D* cam,
        UIManager* manager,
        UIFrame* left,
        UIFrame* right,
        std::function<void (DefaultButton* self,UIRenderer* uirenderer)> updateFnc = [](t2d::ui::DefaultButton* self,t2d::ui::UIRenderer* uirenderer){
                        auto _self = static_cast<t2d::ui::UIAdjusterButton*>(self);
                        if(self->buttonTextPos!=self->position - max::vec2<float>{static_cast<float>(self->buttonText.size()*(8*self->buttonTextScaleModifier.x)),8*self->buttonTextScaleModifier.y}){
                            self->buttonTextPos = self->position - max::vec2<float>{static_cast<float>(self->buttonText.size()*(8*self->buttonTextScaleModifier.x)),8*self->buttonTextScaleModifier.y};
                            self->renderable->dirt();
                        }
                        self->size.y = self->parent->size.y;
                        if (_self->dragging) {
                        bool isMouseHeld = Omnix::Helpers::np_get_data<bool, IDataProvider::__variants>("OmnixMouseModule", {OMNIX_PRESS,OMNIX_MOUSE_LEFT_BUTTON});
                        if (!isMouseHeld) {
                                _self->dragging = false;
                            } else {
                                float currentMouseX = Omnix::Helpers::np_get_data<int, IDataProvider::__variants>("OmnixMouseModule", {OMNIX_MOUSE_POS_X});
                                float deltaX = currentMouseX - _self->lastMouseX;
                        
                                _self->left->size.x += deltaX;
                                self->position.x += deltaX;
                                _self->right->position.x += deltaX;
                                _self->right->size.x -= deltaX;
                        
                                _self->lastMouseX = currentMouseX;
                            }
                        }
                        uirenderer->drawText(self->buttonText, self->buttonTextPos, self->buttonTextScaleModifier);
                        self->renderable->dirt();
                    }
       
    ){
        UIAdjusterButton* button = new UIAdjusterButton(startPos,scale,color);
                
        button->txLoc = txLoc;
        button->clickTxID = clickTxLoc;
        button->hoverTxID = hoverTxLoc;

        button->hoverColor = hoverColor;
        button->clickColor = clickColor;

        button->txCoords  = txCoords;
        button->clickTXCoords = clickTxCoords;
        button->hoverTXCoords = hoverTxCoords;

        button->buttonText = buttonText;
        
        button->buttonTextScaleModifier = buttonTextScaleModifier;

        button->cam = cam;

        button->manager = manager;

        button->updateFnc = updateFnc;

        button->left = left;
        button->right = right;

        button->init();

        return button;
    }
    

    
    


    



    

    

};
#endif // THUNDER_2D_H