#define NOMINMAX

#include "Omnix.h"
#include "OmnixEvents.h"
#include "OmnixModules.h"
#include "OmnixUtil.h"
#include "OmnixWindowBackend.h"
#include "boltlog.h"
#include "glad/wgl.h"
#include "time_utils.h"
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <variant>
#include <Windows.h>
#include <glad/wgl.h>
#include <gl/GL.h>
#include <vector>
#pragma comment(lib,"opengl32.lib")
#include <dwmapi.h>
#pragma comment(lib,"dwmapi.lib")


#include <windef.h>
#include <wingdi.h>
#include <winuser.h>



// NOTE:: OmnixControllerModule start;
INSTALL(Omnix::Defaults::OmnixControllerModule){
    bool vsync_flag = true;
    RESULT(PRE_INIT,ResultParent::EVENT){
        LOG_LIFECYCLE(logger())<<"starting PRE_INIT."<<blENDL;
        OMNIX_STATE = Core::OmnixState::PRE_INIT;
        ADD_PREFIX(BL_COLORIZE("$"+Core::get_state(OMNIX_STATE)+"$", 41));
        OmnixPreInitPhaseEvent pre_init{};
        omnix.eventBus().publish(&pre_init);
        POP_PREFIX();
        LOG_LIFECYCLE(logger())<<"end PRE_INIT."<<blENDL;  
    }
    RESULT(INIT,ResultParent::EVENT){
        LOG_LIFECYCLE(logger())<<"starting INIT."<<blENDL;
        OMNIX_STATE = Core::OmnixState::INIT;
        ADD_PREFIX(BL_COLORIZE("$"+Core::get_state(OMNIX_STATE)+"$", 42));
        OmnixInitPhaseEvent init{};
        omnix.eventBus().publish(&init);
        POP_PREFIX();
        LOG_LIFECYCLE(logger())<<"end INIT."<<blENDL;
    }
    RESULT(POST_INIT,ResultParent::EVENT){
        LOG_LIFECYCLE(logger())<<"starting POST_INIT."<<blENDL;
        OMNIX_STATE = Core::OmnixState::POST_INIT;
        ADD_PREFIX(BL_COLORIZE("$"+Core::get_state(OMNIX_STATE)+"$", 43));
        OmnixPostInitPhaseEvent post_init{vsync_flag};
        omnix.eventBus().publish(&post_init);
        POP_PREFIX();
        LOG_LIFECYCLE(logger())<<"end POST_INIT."<<blENDL;
    }
    RESULT(MAIN,ResultParent::EVENT){
        LOG_LIFECYCLE(logger())<<"starting MAIN_PHASE."<<blENDL;
        OMNIX_STATE = Core::OmnixState::START;
        ADD_PREFIX(BL_COLORIZE("$"+Core::get_state(OMNIX_STATE)+"$", 44));


        ADD_PREFIX(BL_COLORIZE("#MOD",45))
        int index = 0;
        for (auto mod :omnix.getModules()) {
            LOG_LIFECYCLE(logger())<<mod->id().str()<<" "<<index++<<blENDL;
        }
        POP_PREFIX();

        
        Timer timer{};
        const double BASE_PHYSICS_FPS = 60.0;  
        const double targetPhysicsTime = 1.0 / BASE_PHYSICS_FPS;
        const double maxFrameTime = 1.0 / 20.0;  
        
        double accumulator = 0.0;
        double lastFrameTime = timer.elapsed();
        Timer privateTimer {};
        OMNIX_VSYNC_FLAG = vsync_flag;
        float dt;
        while(OMNIX_STATE==Core::OmnixState::START){
          timer.reset();
          
          double _Reset = 0;
          
          OmnixMainPhaseEvent mainEvent{"Omnix.MainPhase", OMNIX_STATE, _Reset,dt};
          omnix.eventBus().publish(&mainEvent);
          OMNIX_FPS = 1.0f/dt;

          dt = timer.elapsed();
          OMNIX_DELTA_TIME = dt;
        }
        POP_PREFIX();

        ADD_PREFIX(DETAIL_BL_COLORIZE("$"+Core::get_state(OMNIX_STATE)+"$", 30,47));
        OMNIX_STATE = Core::OmnixState::STOP;
        LOG_LIFECYCLE(logger())<<"STOP"<<blENDL;
    }
}END_INSTALL

UNINSTALL(Omnix::Defaults::OmnixControllerModule){
    LOG_INFO(logger())<<" uninstalling OmnixControllerModule"<<blENDL;
}END_UNINSTALL

DATA(Omnix::Defaults::OmnixControllerModule,const std::string& key){
    if(key=="Omnix.STATE") return static_cast<const void*>(&OMNIX_STATE);
    if(key=="Omnix.FPS") return static_cast<const void*>(&OMNIX_FPS);
    if(key=="Omnix.TARGET_FPS") return static_cast<const void*>(&OMNIX_TARGET_FPS);
    if(key=="Omnix.DELTA_TIME") return static_cast<const void*>(&OMNIX_DELTA_TIME);
    if(key=="Omnix.ALPHA_VAL") return static_cast<const void*>(&OMNIX_ALPHA_VAL);
    if(key=="Omnix.VSYNC_FLAG") return static_cast<const void*>(&OMNIX_VSYNC_FLAG);
}END_DATA
DATA(Omnix::Defaults::OmnixControllerModule,const std::vector<__variants>& key){

}END_DATA
NP_DATA(Omnix::Defaults::OmnixControllerModule, const std::vector<__variants>& keys){

}END_DATA
// NOTE:: OmnixControllerModule end;



// NOTE:: OmnixWindowModule start;
INSTALL(Omnix::Defaults::OmnixWindowModule){
    OMNIX_EVENT(OmnixPreInitPhaseEvent, pre_init,&omnix){
        LOG_INFO(logger())
        <<"initializing default config for window! \n"
        <<"title:: Omnix\n"
        <<"width:: 1920\n"
        <<"height:: 1080"<<blENDL; 

        //! default config
        backendWindow->windowConfig.addConfig(OMNIX_WINDOW_CONFIG_TITLE, "Omnix");
        backendWindow->windowConfig.addConfig(OMNIX_WINDOW_CONFIG_WIDTH, 1920);
        backendWindow->windowConfig.addConfig(OMNIX_WINDOW_CONFIG_HEIGHT, 1080);


        LOG_DEBUG(logger())<<"publishing event for window config"<<blENDL;
        OmnixWindowModuleConfigEvent winConfigEvent{backendWindow->windowConfig};
        omnix.eventBus().publish(&winConfigEvent);

    };
    OMNIX_EVENT(OmnixInitPhaseEvent, init,&omnix){
        backendWindow->create(omnix);
    };
    OMNIX_EVENT(OmnixMainPhaseEvent,main_phase,&omnix){
        backendWindow->update(omnix,event);
    };
    omnix.eventBus().subscribe(pre_init);
    omnix.eventBus().subscribe(init);
    omnix.eventBus().subscribe(main_phase);
}END_INSTALL

UNINSTALL(Omnix::Defaults::OmnixWindowModule){
    backendWindow->destroy(omnix);
    delete backendWindow;
}END_UNINSTALL

DATA(Omnix::Defaults::OmnixWindowModule,const std::string& key){
    if(key=="Omnix.WINDOW_MODE"){
        return static_cast<const void*>(&windowMode);
    }
}END_DATA

DATA(Omnix::Defaults::OmnixWindowModule,const std::vector<__variants>& key){

}END_DATA

NP_DATA(Omnix::Defaults::OmnixWindowModule, const std::vector<__variants>& keys){
    
}END_DATA
// NOTE:: OmnixWindowModule end;



// NOTE:: OmnixKeyboardModule start;
INSTALL(Omnix::Defaults::OmnixKeyboardModule){
    OMNIX_EVENT(OmnixKeyboardInputEvent, keyevent){
        int keycode = event->keycode;
        int action = event->action_code;
        if(action==1){
            if(keyboard[keycode]){
                repeats[keycode] = true;
            }
            if(!repeats[keycode]){
                just_press[keycode] = true;
            }
            just_press_repeat[keycode] = true;
            if(just_release[keycode]){
                just_release[keycode] = false;
            }
            if(just_release_repeat[keycode]){
                just_release_repeat[keycode] = false;
            }
            keyboard[keycode] = true;
        }
        else if(action==0){
            if(just_press[keycode]){
                just_press[keycode] = false;
            }
            if(just_press_repeat[keycode]){
                just_press_repeat[keycode] = false;
            }
            repeats[keycode] = false;
            if(!repeats[keycode]){
                just_release[keycode] = true;
            }
            just_release_repeat[keycode] = true;
            keyboard[keycode] = false;
        }
    };
    omnix.eventBus().subscribe(keyevent);
}END_INSTALL

UNINSTALL(Omnix::Defaults::OmnixKeyboardModule){

}END_UNINSTALL

DATA(Omnix::Defaults::OmnixKeyboardModule,const std::string& key){

}END_DATA

DATA(Omnix::Defaults::OmnixKeyboardModule,const std::vector<__variants>& keys){
    
}END_DATA



NP_DATA(Omnix::Defaults::OmnixKeyboardModule, const std::vector<__variants>& keys){
    if(std::holds_alternative<int>(keys[0])){
        auto do_key = std::get<int>(keys[0]);
        if(std::holds_alternative<int>(keys[1])){
            int key_key = std::get<int>(keys[1]);
            if(do_key==OMNIX_PRESS){
              return keyboard[key_key];
            }
            if(do_key==OMNIX_JUST_PRESS){
              bool result = just_press[key_key];
              if(just_press[key_key]){
                  just_press[key_key] = false;
              }
              return result;
            }
            if(do_key==OMNIX_JUST_PRESS_REPEAT){
              bool result = just_press_repeat[key_key];
              if(just_press_repeat[key_key]){
                  just_press_repeat[key_key] = false;
              }
              return result;
            }
            if(do_key==OMNIX_JUST_RELEASE){
              bool result = just_release[key_key];
              if(just_release[key_key]){
                  just_release[key_key] = false;
              }
              return result;
            }
            if(do_key==OMNIX_REPEAT){
              return repeats[key_key];
            }
        }
    }
}END_DATA
// NOTE:: OmnixKeyboardModule end;


// NOTE:: OmnixMouseModule start;
INSTALL(Omnix::Defaults::OmnixMouseModule){
    OMNIX_EVENT(OmnixPreInitPhaseEvent, pre_init){
        mouse[OMNIX_MOUSE_LEFT_BUTTON] = false;
        mouse[OMNIX_MOUSE_RIGHT_BUTTON] = false;
        mouse[OMNIX_MOUSE_MIDDLE_BUTTON] = false;
    };
    OMNIX_EVENT(OmnixWindowSizeEvent, window_size){
        screensizex = event->width;
        screensizey = event->height;
    };
    OMNIX_EVENT(OmnixMouseInputEvent, keyevent){

        auto dx = event->extras[OMNIX_MOUSE_DX];
        auto dy = event->extras[OMNIX_MOUSE_DY];
        auto flags = event->extras[OMNIX_MOUSE_FLAGS];
        auto btn_flags = event->extras[OMNIX_MOUSE_BUTTON_FLAGS];
        auto btn_data = event->extras[OMNIX_MOUSE_BUTTON_DATA];
        auto raw_btn = event->extras[OMNIX_MOUSE_RAW_BUTTONS];
        auto extra = event->extras[OMNIX_MOUSE_EXTRA_INFO];
        auto xpos = event->extras[OMNIX_MOUSE_POS_X];
        auto ypos = event->extras[OMNIX_MOUSE_POS_Y];

        this->xPos = xpos;
        this->yPos = ypos;

        this->dx = dx;
        this->dy = dy;


        auto process_button = [&](int button, int down_flag, int up_flag) {
            if (btn_flags & down_flag) {
                if (!mouse_repeat[button]) {
                    mouse_just_press[button] = true;
                }
                mouse_repeat[button] = true;
                mouse_just_press_repeat[button] = true;
                if (mouse_just_release[button]) mouse_just_release[button] = false;
                if (mouse_just_release_repeat[button]) mouse_just_release_repeat[button] = false;
                mouse[button] = true;
            }
            if (btn_flags & up_flag) {
                if (mouse_just_press[button]) mouse_just_press[button] = false;
                if (mouse_just_press_repeat[button]) mouse_just_press_repeat[button] = false;
                mouse_repeat[button] = false;
                if (!mouse_repeat[button]) mouse_just_release[button] = true;
                mouse_just_release_repeat[button] = true;
                mouse[button] = false;
            }
        };

        process_button(OMNIX_MOUSE_LEFT_BUTTON, RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP);
        process_button(OMNIX_MOUSE_RIGHT_BUTTON, RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP);
        process_button(OMNIX_MOUSE_MIDDLE_BUTTON, RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP);

        
    };

    OMNIX_EVENT(OmnixMainPhaseEvent, updateevent){
        for (int i = 0; i < 3; ++i) {
            prevstates[i] = mouse[i];
        }
    };
    
    omnix.eventBus().subscribe(keyevent);
    omnix.eventBus().subscribe(updateevent);
    omnix.eventBus().subscribe(pre_init);
    omnix.eventBus().subscribe(window_size);
}END_INSTALL

UNINSTALL(Omnix::Defaults::OmnixMouseModule){

}END_UNINSTALL

DATA(Omnix::Defaults::OmnixMouseModule,const std::string& key){

}END_DATA

DATA(Omnix::Defaults::OmnixMouseModule,const std::vector<__variants>& keys){
    
}END_DATA



NP_DATA(Omnix::Defaults::OmnixMouseModule, const std::vector<__variants>& keys){
    if(std::holds_alternative<int>(keys[0])){

        auto query = std::get<int>(keys[0]);

        if(keys.size()>1){
            switch (query) {
               case OMNIX_MOUSE_POS_X:{
                   if(std::get<int>(keys[1])==OMNIX_INVERTED){
                       return screensizex-xPos;
                   }
               }
               case OMNIX_MOUSE_POS_Y:{
                   if(std::get<int>(keys[1])==OMNIX_INVERTED){
                       return screensizey-yPos;
                   }
               }
            }
            if(std::holds_alternative<int>(keys[1])){
                int btn = std::get<int>(keys[1]);
                switch (query) {
                    case OMNIX_PRESS:
                        return mouse[btn];
                    case OMNIX_JUST_PRESS: {
                        bool result = mouse_just_press[btn];
                        if(result){
                            mouse_just_press[btn] = false;
                        }
                        return result;
                    }
                    case OMNIX_JUST_PRESS_REPEAT: {
                        bool result = mouse_just_press_repeat[btn];
                        mouse_just_press_repeat[btn] = false;
                        return result;
                    }
                    case OMNIX_JUST_RELEASE: {
                        bool result = mouse_just_release[btn];
                        mouse_just_release[btn] = false;
                        return result;
                    }
                    case OMNIX_REPEAT:
                        return mouse_repeat[btn];
                }
            }
        }else{
            switch (query) {
               case OMNIX_MOUSE_POS_X:
               return xPos;
               case OMNIX_MOUSE_POS_Y:
               return yPos;

               case OMNIX_MOUSE_DX:
               return dx;
               case OMNIX_MOUSE_DY:
               return dy;
            }
        }
        
    }
}END_DATA
// NOTE:: OmnixMouseModule end;



// NOTE:: OmnixOpenGLModule start;
INSTALL(Omnix::Defaults::OpenGLModule){
    OMNIX_EVENT(OmnixWindowSizeEvent, sizeEvent){
        if(isInContext){
            LOG_DEBUG(logger())<<"size changed:: "<<event->height<<" "<<event->width<<blENDL;
            glViewport(0,0,event->width,event->height);
        }
    };
    OMNIX_EVENT(OmnixWindowDefineGraphicApiEvent, defineGraphicApi,&result){
        event->graphic_backend = OpenGLBACKEND;  
    };
    OMNIX_EVENT(OmnixWindowSetGraphicContextEvent, setcxEvent,&result,&omnix){
        if(event->backend==WinCONTEXT){
            auto cx = static_cast<const OpenGLWinContext*>(event->context.get());
            
            cx->hdc = GetDC(cx->hwnd);

            cx->pfd = {};
            cx->pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            cx->pfd.nVersion = 1;
            cx->pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            cx->pfd.iPixelType = PFD_TYPE_RGBA;
            cx->pfd.cColorBits = 32;
            cx->pfd.cDepthBits = 24;
            cx->pfd.iLayerType = PFD_MAIN_PLANE;
            cx->pfd.cAlphaBits = 8;

            
            int pixelFormat = ChoosePixelFormat(cx->hdc, &cx->pfd);
            if (pixelFormat == 0) {
                result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"Failed to set pixel format"};
            }
    
            if (!SetPixelFormat(cx->hdc, pixelFormat, &cx->pfd)) {
                result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"Failed to set pixel format"};
            }
    
            cx->hglrc = wglCreateContext(cx->hdc);
            if (!cx->hglrc) {
                result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"Failed to create dummy OpenGL rendering context"};
            }
    
            if (!wglMakeCurrent(cx->hdc, cx->hglrc)) {
                result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"Failed to make dummy OpenGL context current"};
            }

            auto wglCreateContextAttribsARB =(PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");

            int attribs[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                WGL_CONTEXT_MINOR_VERSION_ARB, 6,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };

            HGLRC realContext = wglCreateContextAttribsARB(cx->hdc,0,attribs);

            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(cx->hglrc);

            wglMakeCurrent(cx->hdc, realContext);
            cx->hglrc = realContext;
           
            if(!gladLoaderLoadGL()){
                LOG_FATAL(logger())<<"cannot load glad loader"<<FLAG(blALLOCATE)<<blENDL;
            }else{
                auto version = glGetString(GL_VERSION);
                LOG_DEBUG(logger())<<"OpenGL VERSION:: "<<(const char*)version<<FLAG(blALLOCATE)<<blENDL; 
            }
            
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback([](GLenum source, GLenum type, GLuint id,
                                          GLenum severity, GLsizei length,
                                          const GLchar* message, const void* userParam) {
                   std::cout<<"OPENGL:: "<<message<<std::endl;
            }, nullptr);
            
            
            OmnixInitOnGraphicContextEvent __iogce{};
            __iogce.context = event->context;
            __iogce.graphics_backend = OpenGLBACKEND;
            omnix.eventBus().publish(&__iogce);


            std::vector<OmnixResultContext> cxs;
            if(result.hasError(cxs)){
                 LOG_FATAL(logger())<<"has error on results <?> {\n";
                 for(auto cx:cxs){
                     logger()<<": "<<cx.msg<<"\n";
                 }
                 logger()<<blENDL;
                 isInContext = false;
            }else{
               isInContext = true;
            }
        }
    };

    OMNIX_EVENT(OmnixWindowRenderEvent,renderevent,&result,&omnix){
            auto cx = static_cast<const OpenGLWinContext*>(event->context.get());
            auto dt = Helpers::get_data<double>("OmnixControllerModule","Omnix.DELTA_TIME");
            auto vsync = Helpers::get_data<bool>("OmnixControllerModule","Omnix.VSYNC_FLAG");


            OmnixRenderEvent renderEvent;
            renderEvent.context = event->context;
            renderEvent.dt = double(*dt);
            renderEvent.graphics_backend = OpenGLBACKEND;
            omnix.eventBus().publish(&renderEvent);

            
            SwapBuffers(cx->hdc);
    };


    omnix.eventBus().subscribe(defineGraphicApi);
    omnix.eventBus().subscribe(setcxEvent);
    omnix.eventBus().subscribe(renderevent);
    omnix.eventBus().subscribe(sizeEvent);
}END_INSTALL

UNINSTALL(Omnix::Defaults::OpenGLModule){

}END_UNINSTALL

DATA(Omnix::Defaults::OpenGLModule,const std::string& key){

}END_DATA

DATA(Omnix::Defaults::OpenGLModule,const std::vector<__variants>& key){

}END_DATA

NP_DATA(Omnix::Defaults::OpenGLModule, const std::vector<__variants>& keys){

}END_DATA
// NOTE:: OmnixOpenGLModule end;


// NOTE:: OmnixMultiThreadingModule start;
INSTALL(Omnix::Defaults::OmnixMultiThreadingModule){
    OMNIX_EVENT(OmnixPreInitPhaseEvent, preinit,&omnix){
        OmnixRegisterThreadEvent __rte{threads};
        omnix.eventBus().publish(&__rte);

        std::vector<std::function<void()>> workers;
        OmnixRegisterWorkerEvent __rwe{workers};
        omnix.eventBus().publish(&__rwe);

        for (auto _worker:workers)
        {
            std::shared_ptr<std::thread> __nthread =std::make_shared<std::thread>([_worker](){
                _worker();
            });
            __rte.addThread(__nthread);
        }
    };
    omnix.eventBus().subscribe(preinit);

    OMNIX_EVENT(OmnixMainPhaseEvent,mainphaseevent, &omnix){
        
    };
    omnix.eventBus().subscribe(mainphaseevent);
}END_INSTALL

UNINSTALL(Omnix::Defaults::OmnixMultiThreadingModule){
    for (auto _thread:threads)
    {
        if (_thread)
        {
            if(_thread->joinable()){
                _thread->join();
            }
        }
    }
    
}END_UNINSTALL

DATA(Omnix::Defaults::OmnixMultiThreadingModule,const std::string& key){
}END_DATA

DATA(Omnix::Defaults::OmnixMultiThreadingModule,const std::vector<__variants>& key){
}END_DATA

NP_DATA(Omnix::Defaults::OmnixMultiThreadingModule, const std::vector<__variants>& keys){
}END_DATA
// NOTE:: OmnixMultiThreadingModule end;

#include <thunder2d.h>
// NOTE:: OmnixUIModule start;
INSTALL(Omnix::Defaults::OmnixUIModule){
    OMNIX_EVENT(OmnixPreInitPhaseEvent, preinit,&omnix){
    };
    omnix.eventBus().subscribe(preinit);
    OMNIX_EVENT(OmnixMainPhaseEvent,mainphaseevent, &omnix){
    };
    omnix.eventBus().subscribe(mainphaseevent);
}END_INSTALL

UNINSTALL(Omnix::Defaults::OmnixUIModule){
    
}END_UNINSTALL

DATA(Omnix::Defaults::OmnixUIModule,const std::string& key){
}END_DATA

DATA(Omnix::Defaults::OmnixUIModule,const std::vector<__variants>& key){
}END_DATA

NP_DATA(Omnix::Defaults::OmnixUIModule, const std::vector<__variants>& keys){
}END_DATA
// NOTE:: OmnixUIModule end;
