#ifndef OMNIX_MODULES_H
#define OMNIX_MODULES_H

#include "BoltID.h"
#include "Omnix.h"
#include "OmnixEvents.h"
#include "OmnixModuleID.h"
#include "OmnixUtil.h"
#include "boltlog.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <array>
#include <thread>
#include <typeinfo>
#include <variant>
#include <Windows.h>
#include <vector>
#include <windef.h>
namespace t2d::ui{
    class UIRenderer;
    struct UIElement;
    class UIManager;
};

struct OmnixPhaseEvent:public OmnixEvent{
};
struct OmnixPreInitPhaseEvent:public OmnixPhaseEvent{
    std::string name;
};
struct OmnixInitPhaseEvent:public OmnixPhaseEvent{
    std::string name;
};
struct OmnixPostInitPhaseEvent:public OmnixPhaseEvent{
    std::string name;
    bool& vsync;
    OmnixPostInitPhaseEvent(bool& vsync):vsync(vsync){}
};

struct OmnixMainPhaseEvent:public OmnixPhaseEvent{
    public:
    std::string name;
    Omnix::Core::OmnixState &OmnixRunState;
    double &target_fps;
    double dt;
    OmnixMainPhaseEvent(const std::string& name,Omnix::Core::OmnixState &OmnixRunState,double &target_fps,double dt)
    :name(name),
    OmnixRunState(OmnixRunState),
    target_fps(target_fps),dt(dt){}
};

namespace Omnix::Core
{
    enum class OmnixState{
        PRE_INIT,
        INIT,
        POST_INIT,
        START,
        STOP
    };
    inline std::string get_state(const OmnixState& state){
        switch (state) {
            case OmnixState::PRE_INIT : return "PRE-INIT";
            case OmnixState::INIT : return "INIT";
            case OmnixState::POST_INIT : return "POST-INIT";
            case OmnixState::START : return "START";
            case OmnixState::STOP : return "STOP";
        }
    };
}//Omnix::Core


struct OmnixFrameEndEvent:public OmnixEvent{
};
namespace Omnix::Defaults{
class OmnixControllerModule:public Omnix::Core::OmnixModule
{
    std::mutex m;
    std::shared_ptr<std::thread> _render_thread_;
    Omnix::Core::OmnixState OMNIX_STATE = Omnix::Core::OmnixState::PRE_INIT;
    double OMNIX_FPS = 0.0;
    double OMNIX_TARGET_FPS = 0.0;
    double OMNIX_DELTA_TIME = 0.0;
    double OMNIX_ALPHA_VAL = 0.0;
    bool OMNIX_VSYNC_FLAG =false;
    public:
    OmnixControllerModule():Omnix::Core::OmnixModule(OmnixModuleID::newModuleID("OmnixController","controls omnix phases and core logics.")){}
    
    
    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;
};
struct IWindowConfig{
    std::map<std::string,std::variant<int,bool,std::string>> config{};

    template<typename conf>
    void addConfig(const std::string& id,const conf& val){
        config[id] = val;
    }

    template<typename conf>
    conf getConfig(const std::string& id){
        if(std::holds_alternative<conf>(config[id])){
            auto rtrn = std::get<conf>(config[id]);
            return rtrn;
        }else{
            throw new std::bad_cast();
        }
    }
};

#define OMNIX_WINDOW_CONFIG_TITLE "Omnix.WinC:WIN_TITLE"
#define OMNIX_WINDOW_CONFIG_WIDTH "Omnix.WinC:WIN_WIDTH"
#define OMNIX_WINDOW_CONFIG_HEIGHT "Omnix.WinC:WIN_HEIGHT"
enum class OmnixWindowMode {
    WINDOWED,
    FULLSCREEN,
    BORDERLESS_FULLSCREEN
};

struct OmnixSetWindowModeEvent : public OmnixEvent {
    OmnixWindowMode mode;
    OmnixSetWindowModeEvent(OmnixWindowMode mode)
        : mode(mode) {}
};
class IWindow{
    const BL::Default::Logger* logger;
    Omnix::Core::Omnix& _omnix;
    public:
    IWindowConfig windowConfig;
    std::shared_ptr<OmnixWindowModule> frontend;
    IWindow(Omnix::Core::Omnix& omnix):_omnix(omnix),windowConfig({}){}
    inline void set_logger(const BL::Default::Logger* pointer){logger=pointer;}
    inline Omnix::Core::Omnix& omnix(){return _omnix;}
    virtual ~IWindow() = default;
    virtual OmnixResult create(Omnix::Core::Omnix& omnix)=0;
    virtual OmnixResult update(Omnix::Core::Omnix& omnix,OmnixMainPhaseEvent* updateEvent)=0;
    virtual OmnixResult destroy(Omnix::Core::Omnix& omnix)=0;
};

struct OmnixWindowModuleConfigEvent:public OmnixEvent{
    IWindowConfig& config;
    OmnixWindowModuleConfigEvent(IWindowConfig& config):config(config){}
};

struct WinContext{
    public:
    HWND& hwnd;
    HDC& hdc;
};

struct OpenGLWinContext:public WinContext{
    public:
    HGLRC& hglrc;
    PIXELFORMATDESCRIPTOR& pfd;
};

#define OpenGLBACKEND 1197
#define VulkanBACKEND 1198
#define DirectXBACKEND 1199
struct OmnixWindowDefineGraphicApiEvent:public OmnixEvent{
    public:
    int& graphic_backend;
    OmnixWindowDefineGraphicApiEvent(int& backend):graphic_backend(backend){}
};


struct OmnixWindowRenderEvent:public OmnixEvent{
    std::shared_ptr<WinContext> context;
    const int backend;
    OmnixWindowRenderEvent(std::shared_ptr<WinContext> context,const int& backend):context(context),backend(backend){}
};
struct OmnixWindowSizeEvent:public OmnixEvent{
    int width;
    int height;
    OmnixWindowSizeEvent(const int& width,int& height):width(width),height(height){}
};

#define WinCONTEXT 12001
#define LinuxCONTEXT 12002
#define MacCONTEXT 12003
struct OmnixWindowSetGraphicContextEvent:public OmnixEvent{
    std::shared_ptr<WinContext> context;
    const int backend;
    OmnixWindowSetGraphicContextEvent(std::shared_ptr<WinContext> context,const int& backend):context(context),backend(backend){}
};

class OmnixWindowModule:public Omnix::Core::OmnixModule{
    private:
    IWindow *backendWindow;
    public:
    OmnixWindowModule(IWindow* backend):Omnix::Core::OmnixModule(OmnixModuleID::newModuleID("OmnixWindow","window management and input.")),backendWindow(backend){}


    OmnixWindowMode windowMode = OmnixWindowMode::WINDOWED;

    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;

};

enum class OmnixInputType{
    KEYBOARD,
    MOUSE
};





struct OmnixInputEvent:public OmnixEvent{
    public:
    std::map<int, int> extras;
    OmnixInputType type;
    int keycode;
    int action_code;
    unsigned short specs;
    long long lParam;
    OmnixInputEvent(const OmnixInputType& type,const int& keycode,const int& action_code,const unsigned short& specs,long long &lParam):
    type(type),
    keycode(keycode),
    action_code(action_code),
    specs(specs),
    lParam(lParam){}
};

struct OmnixKeyboardInputEvent:public OmnixInputEvent{
    OmnixKeyboardInputEvent(const OmnixInputType& type,const int& keycode,const int& action_code,const unsigned short& specs,long long &lParam):
    OmnixInputEvent(type,keycode,action_code,specs,lParam){}
};

struct OmnixMouseInputEvent:public OmnixInputEvent{
    OmnixMouseInputEvent(const OmnixInputType& type,const int& keycode,const int& action_code,const unsigned short& specs,long long &lParam):
    OmnixInputEvent(type,keycode,action_code,specs,lParam){}
};



#define OMNIX_PRESS 1
#define OMNIX_JUST_PRESS 2
#define OMNIX_JUST_PRESS_REPEAT 3
#define OMNIX_RELEASE 4
#define OMNIX_JUST_RELEASE 5
#define OMNIX_REPEAT 6

#define OMNIX_MOUSE_DX 10001
#define OMNIX_MOUSE_DY 10002
#define OMNIX_MOUSE_FLAGS 10003
#define OMNIX_MOUSE_BUTTON_FLAGS 10004
#define OMNIX_MOUSE_BUTTON_DATA 10005
#define OMNIX_MOUSE_RAW_BUTTONS 10006
#define OMNIX_MOUSE_EXTRA_INFO 10007
#define OMNIX_MOUSE_POS_X 10008
#define OMNIX_MOUSE_POS_Y 10009

#define OMNIX_INVERTED 20001

#define OMNIX_MOUSE_LEFT_BUTTON 0
#define OMNIX_MOUSE_RIGHT_BUTTON 1
#define OMNIX_MOUSE_MIDDLE_BUTTON 2


class OmnixKeyboardModule:public Core::OmnixModule{
    
    public:
    OmnixKeyboardModule():Omnix::Core::OmnixModule(
        OmnixModuleID::newModuleID(
            "OmnixKeyboardModule",
            "subscribes to keyboard events and organizes them.",{OmnixDependency{OmnixDependencyType::MODULE,"OmnixWindowModule"}})){
            }



    mutable std::unordered_set<std::string> just_press_consumed[512];

    bool keyboard[256]{};
    mutable bool just_press[256]{};
    mutable bool just_release[256]{};

    mutable bool just_press_repeat[256]{};
    mutable bool just_release_repeat[256]{};

    bool repeats[256]{};
    
    
    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;

    
};




class OmnixMouseModule:public Core::OmnixModule{
    public:
    OmnixMouseModule():Omnix::Core::OmnixModule(
        OmnixModuleID::newModuleID(
            "OmnixKeyboardModule",
            "subscribes to mouse events and organizes them.",{OmnixDependency{OmnixDependencyType::MODULE,"OmnixWindowModule"}})){
            }

    int screensizex;
    int screensizey;

    mutable bool mouse[3]{false,false,false};
    mutable bool prevstates[3]{false,false,false};

    mutable bool mouse_just_press[3]{};
    mutable bool mouse_just_release[3]{};
    mutable bool mouse_just_press_repeat[3]{};
    mutable bool mouse_just_release_repeat[3]{};
    mutable bool mouse_repeat[3]{};

    
    int xPos=0.0f,yPos=0.0f;
    int dx=0.0f,dy=0.0f;


    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;

};


struct OmnixInitOnGraphicContextEvent:public OmnixEvent{
    std::shared_ptr<WinContext> context;
    int graphics_backend;
};
struct OmnixRenderEvent:public OmnixEvent{
    std::shared_ptr<WinContext> context;
    float dt;
    int graphics_backend;
};

class OpenGLModule:public Core::OmnixModule{
    bool isInContext = false;
    public:
    OpenGLModule():Omnix::Core::OmnixModule(
        OmnixModuleID::newModuleID(
            "OpenGL",
            "opengl",{OmnixDependency{OmnixDependencyType::MODULE,"OmnixWindowModule"}})){
            }
    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;

};
struct OmnixRegisterThreadEvent:public OmnixEvent{
    std::vector<std::shared_ptr<std::thread>>& __threads;

    OmnixRegisterThreadEvent(std::vector<std::shared_ptr<std::thread>>& __threads):__threads(__threads){}

    inline void addThread(std::shared_ptr<std::thread>& thread){
        __threads.push_back(thread);
    }
};
struct OmnixRegisterWorkerEvent:public OmnixEvent{
    std::vector<std::function<void()>>& __workers;

    OmnixRegisterWorkerEvent(std::vector<std::function<void()>>& __workers):__workers(__workers){}

    inline void addWorker(std::function<void()> worker){
        __workers.push_back(worker);
    }
};
class OmnixMultiThreadingModule:public Core::OmnixModule{
    std::vector<std::shared_ptr<std::thread>> threads;
    public:
    OmnixMultiThreadingModule():Omnix::Core::OmnixModule(
        OmnixModuleID::newModuleID(
            "OmnixMultiThreadingModule",
            "organizes multiple threads",{})){}

    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;
};

class OmnixSceneModule:public Core::OmnixModule{
    OmnixSceneModule():Omnix::Core::OmnixModule(
        OmnixModuleID::newModuleID(
            "OmnixSceneModule",
            "default scene management for omnix.",{})){}
    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;
};



#define OMNIX_UI_ELEMENT 50001 
 #define OMNIX_UI_ELEMENT_BUTTON 50011
 #define OMNIX_UI_ELEMENT_ADJUSTERBUTTON 50021 
 #define OMNIX_UI_ELEMENT_SLIDER 50031
 #define OMNIX_UI_ELEMENT_CHECKBOX 50041



#define OMNIX_UI_ELEMENT_CLICK 50101
#define OMNIX_UI_ELEMENT_HOLD 50201
#define OMNIX_UI_ELEMENT_RELEASE 50301


struct OmnixUIEvent:OmnixEvent{
    int eventType = 0;
    int elementType = -1;
    int action = -1;
    std::vector<int> extras_int;
    std::vector<std::string> extras_str;
    std::map<int, int> infos;
    BoltID& id;
    OmnixUIEvent(int eventType,int elementType,int action,BoltID& id):eventType(eventType),elementType(elementType),action(action),id(id){}
};

class OmnixUIModule:public Core::OmnixModule{
    OmnixUIModule():Omnix::Core::OmnixModule(
        OmnixModuleID::newModuleID(
            "OmnixUIModule",
            "default ui management for omnix.",{})){}
    OmnixResult install(Omnix::Core::Omnix& omnix) override;
    OmnixResult uninstall(Omnix::Core::Omnix& omnix) override;

    const void* getData(const std::string& key) const override;
    const void* getData(const std::vector<__variants>& key) const override;
    const __variants np_getData(const std::vector<__variants>& keys,const BoltID* id = nullptr) const override;
};

}



#endif // OMNIX_MODULES_H