#include "OmnixData.h"
#include "fwd.hpp"
#include "gtc/type_ptr.hpp"
#include "id.h"
#include "math_functions.h"
#include "types.h"
#include <cstddef>
#include <cstdlib>
#include <malloc.h>
#include <stdalign.h>
#define NOMINMAX
#include "max.h"
#include "t2dshader.h"
#include "time_utils.h"
#include "token_utils.h"
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#define STB_IMAGE_IMPLEMENTATION

#include "Omnix.h"
#include "OmnixEvents.h"
#include "OmnixModuleID.h"
#include "OmnixModules.h"
#include "OmnixUtil.h"
#include "boltlog.h"
#include <fstream>
#include <brain.h>
#include <memory>
#include <string>
#include <test_utils.h>
#include <vector>
#include "OmnixWindowBackend.h"
#include "glad/gl.h"
#include "texture.h"
#include <boltf.h>
#include <thunder2d.h>
#include <batch.h>

#include <box2d.h>
#include <unordered_map>
#include <utility>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_write.h"
static Omnix::Core::OmnixState STATIC_STATE = Omnix::Core::OmnixState::START;

void SaveTextureToFile(GLuint textureID, int width, int height, const char* filename) {
    glBindTexture(GL_TEXTURE_2D, textureID);

    std::vector<unsigned char> pixels(width * height * 4); 
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<unsigned char> flipped(pixels.size());
    for (int y = 0; y < height; ++y)
        memcpy(&flipped[y * width * 4], &pixels[(height - 1 - y) * width * 4], width * 4);

    stbi_write_png(filename, width, height, 4, flipped.data(), width * 4);
}
std::string formatFloat(float value, int precision = 1) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}


enum class Side{
    RIGHT,LEFT
};
constexpr static int pixels_per_meter = 64.0f;
typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

void enableVSync(bool enable) {


    if (!wglSwapIntervalEXT) {
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    }

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(enable ? 1 : 0);
    } else {
        OutputDebugStringA("wglSwapIntervalEXT not supported\n");
    }
}

class TestModule:public Omnix::Core::OmnixModule{
    public:
    t2d::ui::UIManager* manager;
    int width=0,height = 0;
    Texture tex{};
    Texture tex2{};
    Timer anim{};
    bool setblend = false;

    t2d::ui::UIFrame* frame;
    std::vector<std::array<max::vec2<float>, 4>> uicoords = parse_sheet({160,160}, {16,16});

    
    int obj;

    t2d::ui::UIRenderer *uirenderer;
    
    int mousex = 0;
    int mousey = 0;

    std::vector<GLint> __uitexs;
    

    t2d::ui::UIFont *font;
    
    Camera2D cam {600,600};
    Map<50, 50> *map;

    Scene* scene;


    t2d::ui::UIFrame* frame2;
    t2d::ui::UIFrame* frame3;
    t2d::ui::UIFrame* frame4;

    t2d::ui::UIAdjusterButton* adjuster;

    std::vector<GLint> spriteTextures;
    bool dragging = false;
    int lastMouseX = 0;

    TestModule():Omnix::Core::OmnixModule(OmnixModuleID::newModuleID("TestModule","for testing Omnix.")){}
    INSTALL(TestModule){
       


        //! testing the auto event macros
        OMNIX_EVENT_AUTO(Omnix::Defaults::OmnixWindowModuleConfigEvent)
        event->config.addConfig(OMNIX_WINDOW_CONFIG_HEIGHT, 600);
        event->config.addConfig(OMNIX_WINDOW_CONFIG_WIDTH, 600);
        event->config.addConfig(OMNIX_WINDOW_CONFIG_TITLE, "TestModule");
        OMNIX_EVENT_END_AUTO("TestModule")

        

        
       

        OMNIX_EVENT(Omnix::Defaults::OmnixWindowSizeEvent, sizeEvent){
            width = event->width;
            height = event->height;

            cam.setViewportSize((float)event->width,(float)event->height);

        };

        OMNIX_EVENT(OmnixPreInitPhaseEvent, pre_init_event){
            
        };
        OMNIX_EVENT(OmnixInitPhaseEvent, init_event){
        };
        OMNIX_EVENT(OmnixPostInitPhaseEvent, post_init_event){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            event->vsync =false;
        };

        OMNIX_EVENT(Omnix::Defaults::OmnixInputEvent, inputEvent){
           
        };
        omnix.eventBus().subscribe(inputEvent);
        
        OMNIX_EVENT(OmnixMainPhaseEvent, main_phase_event,&omnix){ 

            event->OmnixRunState = STATIC_STATE;


            
            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_PRESS,VK_RIGHT})){
                cam.move({5.0f*event->dt*pixels_per_meter,0});
            }
            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_PRESS,VK_LEFT})){
                cam.move({-5.0f*event->dt*pixels_per_meter,0});
            }
            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_PRESS,VK_UP})){
                cam.move({0,5.0f*event->dt*pixels_per_meter});
            }
            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_PRESS,VK_DOWN})){
                cam.move({0,-5.0f*event->dt*pixels_per_meter});
            }

            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_PRESS,'D'})){
                b2Body_SetLinearVelocity(scene->objects[obj].body, {3,0});
            }
            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_PRESS,'A'})){
                b2Body_SetLinearVelocity(scene->objects[obj].body, {-3,0});
            }


            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_JUST_PRESS,'P'})){
                cam.zoomBy(0.7);
            }
            
            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_JUST_PRESS,'O'})){
                cam.zoomBy(1.2);
            }

            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {OMNIX_JUST_PRESS,{VK_SPACE}})){
                frame2->size.x+=10.0f;
            }

            
            if(Omnix::Helpers::np_get_data<bool,__variants>("OmnixKeyboardModule", {{OMNIX_JUST_PRESS},{VK_F11}})){
                if((*Omnix::Helpers::get_data<Omnix::Defaults::OmnixWindowMode>("OmnixWindowModule","Omnix.WINDOW_MODE")==Omnix::Defaults::OmnixWindowMode::BORDERLESS_FULLSCREEN)){
                    Omnix::Defaults::OmnixSetWindowModeEvent __wm{Omnix::Defaults::OmnixWindowMode::WINDOWED};
                    omnix.eventBus().publish(&__wm);
                }
                else if((*Omnix::Helpers::get_data<Omnix::Defaults::OmnixWindowMode>("OmnixWindowModule","Omnix.WINDOW_MODE")==Omnix::Defaults::OmnixWindowMode::WINDOWED)){
                    Omnix::Defaults::OmnixSetWindowModeEvent __wm{Omnix::Defaults::OmnixWindowMode::BORDERLESS_FULLSCREEN};
                    omnix.eventBus().publish(&__wm);
                }
            }


            


        };
        
        OMNIX_EVENT(Omnix::Defaults::OmnixInitOnGraphicContextEvent, initGraphicEvent,&omnix) {
            if(event->graphics_backend == OpenGLBACKEND){
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);


                manager = new t2d::ui::UIManager(omnix);

                enableVSync(true);


                BoltF::getInstance().init("testfont.ttf", 25);



                GLuint txhold = 0;
 
                font = new t2d::ui::UIFont();
                font->chars = t2d::ui::UIFont::init("testfont.ttf", 32, &txhold);
                font->fontAtlasTexture = txhold;

                SaveTextureToFile(txhold, 512, 512, "atlas.png");

                tex.loadFromFile("resources/ui/uiatlas.png");

                spriteTextures =  {static_cast<int>(font->fontAtlasTexture),static_cast<int>(tex.getID())};
                
                
                uirenderer = new t2d::ui::UIRenderer({600,600},spriteTextures);

                uirenderer->fonts.push_back(font);
                

                
                scene = new Scene(cam);
                scene->init({0.0f,-10.0f},{tex.getID()});
               
                map = Map<50,50>::initialize_and_create_map_HEAP(scene->spriteBatch, {}, {2500,2500}, {50,50});
                map->m_state(0, SOLID);
                map->m_color(0, {1,0,0,1});

                scene->init_map(map);

                obj = scene->add_object_dynamic({50,50}, {50,50});
                scene->objects[obj].sprite->setZOrder(100);
                scene->objects[obj].sprite->color = {0,1,0,1};
                scene->spriteBatch->reload();



                frame2 = new t2d::ui::UIFrame({0,0},{955,static_cast<float>(height)},{1,0,1,1});
                frame2->updateFnc = [this](t2d::ui::UIFrame* self){
                    if(self->size.y!=height){
                        self->size.y = height;
                        self->renderable->dirt();
                    }
                    self->renderable->dirt();
                };
                frame2->layout = new t2d::ui::HorizontalLayout();

                frame3 = new t2d::ui::UIFrame({0,0},{955,static_cast<float>(height)},{1,0,1,1});
                frame3->updateFnc = [this](t2d::ui::UIFrame* self){
                    if(self->size.y!=height){
                        self->size.y = height;
                        self->renderable->dirt();
                    }
                    self->renderable->dirt();
                };
                frame3->layout = new t2d::ui::HorizontalLayout();

                


                frame = new t2d::ui::UIFrame({0,0},{1920,1080},{0,1,1,1});

                frame3->add(t2d::ui::newButton(
                    {0,0},
                    {150,50},
                    1, 
                    1, 
                    1, 
                    {1,0,0,1},
                    {0.8,0,0,1},
                    {0.5,0.0,0.0,1},
                    uicoords[0],
                    uicoords[1],
                    uicoords[0],
                    "Button1",
                    {0.5,0.5},
                    &cam,
                    manager
                    ));
                    
                frame2->add(t2d::ui::newButton(
                    {0,0},
                    {150,50},
                    1, 
                    1, 
                    1, 
                    {1,0,0,1},
                    {0.8,0,0,1},
                    {0.5,0.0,0.0,1},
                    uicoords[0],
                    uicoords[1],
                    uicoords[0],
                    "Button1",
                    {0.5,0.5},
                    &cam,
                    manager
                    ));
                
               

                frame->add(frame2);
                frame->add(t2d::ui::newAdjusterButton(
                    {0,0},
                    {10,50},
                    -1, 
                    -1, 
                    -1, 
                    {1,0,0,1},
                    {0.8,0,0,1},
                    {0.5,0.0,0.0,1},
                    uicoords[0],
                    uicoords[1],
                    uicoords[0],
                    "",
                    {0.5,0.5},
                    &cam,
                    manager,
                    frame2,
                    frame3
                    ));
                frame->add(frame3);
                

                frame->updateFnc = [this](t2d::ui::UIFrame* self){
                    if(self->size.x!=width){
                        self->size.x = width;
                        self->renderable->dirt();
                    }
                    if(self->size.y!=height){
                        self->size.y = height;
                        self->renderable->dirt();
                    }
                    if(self->position!=max::vec2<float>{static_cast<float>(width/2),static_cast<float>(height/2)}){
                        self->position=max::vec2<float>{static_cast<float>(width/2),static_cast<float>(height/2)};
                        self->renderable->dirt();
                    }
                };

                frame->layout = new t2d::ui::HorizontalLayout();

                
                
                manager->elements.push_back(frame);
            
                max::vec2<int> mvec {mousex,mousey};

                uirenderer->renderer->addRenderable(std::make_shared<t2d::ui::UIGlyph>(font->chars,'X',max::vec2<float>{0,0},max::vec2<float>{0,0}));
                manager->draw(uirenderer);
                uirenderer->refresh();
            }
        };
        
        OMNIX_EVENT(Omnix::Defaults::OmnixRenderEvent, renderEvent,&omnix) {
            if(event->graphics_backend == OpenGLBACKEND){
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                mousex = Omnix::Helpers::np_get_data<int,__variants>("OmnixMouseModule", {{OMNIX_MOUSE_POS_X}});
                mousey = Omnix::Helpers::np_get_data<int,__variants>("OmnixMouseModule", {{OMNIX_MOUSE_POS_Y,OMNIX_INVERTED}});
                max::vec2<int> mvec {mousex,mousey};


                map->refresh();
                scene->update(std::min(1.0f/30.0f,event->dt));

                


                uirenderer->begin();
                uirenderer->drawText("FPS:: "+formatFloat(1.0f/event->dt), {0,40}, {1,1},{1,0,0,1});
                uirenderer->end(&cam);

                std::cout<<uirenderer->renderer->renderables.size()<<std::endl;


                manager->update(uirenderer);

            
            }
        };

        omnix.eventBus().subscribe(pre_init_event);
        omnix.eventBus().subscribe(init_event);
        omnix.eventBus().subscribe(post_init_event);
        omnix.eventBus().subscribe(main_phase_event);
        omnix.eventBus().subscribe(renderEvent);
        omnix.eventBus().subscribe(initGraphicEvent);
        omnix.eventBus().subscribe(sizeEvent);
    }END_INSTALL

    UNINSTALL(TestModule){

        uirenderer->dispose();
        delete uirenderer;
        manager->dispose();
        delete manager;


        delete map;
        delete scene;
    }END_UNINSTALL

    const void* getData(const std::string& key)const override{
        return 0;
    };
    const void* getData(const std::vector<__variants>& keys)const override{
        return 0;
    };
    const __variants np_getData(const std::vector<__variants>& keys) const override{
        return {};
    }
};

TEST(_BLogTest){
    Omnix::Core::Omnix omnix;

    auto Test_mod = std::make_shared<TestModule>();
    Omnix::Core::Omnix::data_instance().registerProvider("TestModule",Test_mod);

    auto Controller_mod = std::make_shared<Omnix::Defaults::OmnixControllerModule>();
    Omnix::Core::Omnix::data_instance().registerProvider("OmnixControllerModule", Controller_mod);
    
    Omnix::Defaults::IWindow *win;
    #ifdef _WIN32
    win = new Omnix::Backends::WindowsWindowBackend(omnix);
    #endif
    
    
    auto Window_mod = std::make_shared<Omnix::Defaults::OmnixWindowModule>(win);
    win->set_logger(&Window_mod->logger());
    win->frontend = Window_mod;
    Omnix::Core::Omnix::data_instance().registerProvider("OmnixWindowModule", Window_mod);

    auto Keyboard_mod = std::make_shared<Omnix::Defaults::OmnixKeyboardModule>();
    Omnix::Core::Omnix::data_instance().registerProvider("OmnixKeyboardModule",Keyboard_mod);

    auto Mouse_mod = std::make_shared<Omnix::Defaults::OmnixMouseModule>();
    Omnix::Core::Omnix::data_instance().registerProvider("OmnixMouseModule",Mouse_mod);

    auto OpenGL_mod = std::make_shared<Omnix::Defaults::OpenGLModule>();
    Omnix::Core::Omnix::data_instance().registerProvider("OpenGLModule",OpenGL_mod);

    omnix.getModules().push_back(Test_mod);
    omnix.getModules().push_back(Window_mod);
    omnix.getModules().push_back(OpenGL_mod);
    omnix.getModules().push_back(Keyboard_mod);
    omnix.getModules().push_back(Mouse_mod);
    omnix.getModules().push_back(Controller_mod);

    LOG_INFO(omnix.globalLogger())<<omnix.installModules().toString()<<blENDL;
    LOG_INFO(omnix.globalLogger())<<omnix.uninstallModules().toString()<<blENDL;


    return BoltTestResult::CALCULATED;
}


int main() {
    TIME_PROFILER_IS_ON = true;
    COLORIZED_MODE = true;

    BOLT_TEST(BLogTest, "noDesc", _BLogTest);

    std::ofstream stream{"profilerResult.json"};
    runTests(std::cout,stream);
    stream.close();
    return 0;
}