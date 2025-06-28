#ifndef OMNIX_WINDOW_BACKEND_H
#define OMNIX_WINDOW_BACKEND_H

#include "Omnix.h"
#include "OmnixEvents.h"
#include "OmnixModules.h"
#include "OmnixUtil.h"
#include "test_utils.h"
#include <memory>
#include <minwindef.h>
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>



#ifdef  _WIN32
#include <Windows.h>

#undef ERROR
class Omnix::Backends::WindowsWindowBackend:public Defaults::IWindow{
    public:
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    WNDCLASS wc{};
    int graphic_backend = 0;
    std::shared_ptr<Defaults::WinContext> context;

    WindowsWindowBackend(Omnix::Core::Omnix& omnix):Defaults::IWindow(omnix){}

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        Omnix::Backends::WindowsWindowBackend* self = reinterpret_cast<Omnix::Backends::WindowsWindowBackend*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA));
        switch (uMsg) {
            case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

            case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            Defaults::OmnixWindowSizeEvent __wse{width,height};
            self->omnix().eventBus().publish(&__wse);
            return 0;
            }
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            case WM_ENTERSIZEMOVE:
                break;
    
            case WM_EXITSIZEMOVE:
                break;
    
            case WM_MOUSEMOVE:
                  
                break;
    
            case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                return TRUE;
            }
            break;
    
            case WM_INPUT:
            {
                UINT dwSize = 0;
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
                if (dwSize > 0) {
                    BYTE* lpb = new BYTE[dwSize];
                    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                        RAWINPUT* raw = (RAWINPUT*)lpb;
                        if (raw->header.dwType == RIM_TYPEKEYBOARD) {

                            RAWKEYBOARD& rawKB = raw->data.keyboard;
            
                            USHORT vkey = rawKB.VKey;
            
                            USHORT makeCode = rawKB.MakeCode;
                            
            
                            USHORT flags = rawKB.Flags;
                            bool keyDown = !(flags & RI_KEY_BREAK);
            
                            Defaults::OmnixKeyboardInputEvent event(Defaults::OmnixInputType::KEYBOARD,vkey,keyDown,flags,lParam);

                            self->omnix().eventBus().publish(&event);
                        
                            
                        }
                        if(raw->header.dwType == RIM_TYPEMOUSE){
                            RAWMOUSE& m = raw->data.mouse;
                            Defaults::OmnixMouseInputEvent __ie{
                                Defaults::OmnixInputType::MOUSE,
                                m.usButtonFlags, 
                                -1,
                                m.usButtonData,
                                lParam
                            };

                            POINT pos;
                            GetCursorPos(&pos);
                            ScreenToClient(hwnd, &pos);
                        
                            __ie.extras[OMNIX_MOUSE_DX] = m.lLastX;
                            __ie.extras[OMNIX_MOUSE_DY] = m.lLastY;
                            __ie.extras[OMNIX_MOUSE_FLAGS] = m.usFlags;
                            __ie.extras[OMNIX_MOUSE_BUTTON_FLAGS] = m.usButtonFlags;
                            __ie.extras[OMNIX_MOUSE_BUTTON_DATA] = m.usButtonData;
                            __ie.extras[OMNIX_MOUSE_RAW_BUTTONS] = m.ulRawButtons;
                            __ie.extras[OMNIX_MOUSE_EXTRA_INFO] = m.ulExtraInformation;
                            __ie.extras[OMNIX_MOUSE_POS_X] = pos.x;
                            __ie.extras[OMNIX_MOUSE_POS_Y] = pos.y;

                            self->omnix().eventBus().publish(&__ie);
                        }
                    }
                    delete[] lpb;
                }
                return 0;
            }
            default:
                break;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    RESULT_FUNC(create,WindowsWindowBackend,Omnix::Core::Omnix& omnix){
        
        OMNIX_EVENT(Omnix::Defaults::OmnixSetWindowModeEvent, windowModeEvent) {
            auto cx = context;
            HWND hwnd = cx->hwnd;
        
            if (!hwnd) return;
        
            DEVMODE dm;
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
        
            switch (event->mode) {
                case Omnix::Defaults::OmnixWindowMode::WINDOWED: {
                    SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                    SetWindowPos(hwnd, HWND_NOTOPMOST, 100, 100, 800, 600, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                    break;
                }
                case Omnix::Defaults::OmnixWindowMode::FULLSCREEN: {
                    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, dm.dmPelsWidth, dm.dmPelsHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                    break;
                }
                case Omnix::Defaults::OmnixWindowMode::BORDERLESS_FULLSCREEN: {
                    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);
                    SetWindowPos(hwnd, HWND_TOP, 0, 0, dm.dmPelsWidth, dm.dmPelsHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                    break;
                }
            }
           frontend->windowMode = event->mode;
        };
        omnix.eventBus().subscribe(windowModeEvent);


        
        Defaults::OmnixWindowDefineGraphicApiEvent __wdgae(graphic_backend);
        omnix.eventBus().publish(&__wdgae);
        
        
        
        HINSTANCE hInstance = GetModuleHandle(nullptr);

        wc = {};
        wc.lpfnWndProc = WindowProc;
        
        wc.hInstance = hInstance;
        wc.lpszClassName = "OmnixWindowClass";

        if (!RegisterClass(&wc)) {
            result<<OmnixResultContext{OmnixResultContextLevel::TRACE,""};
            result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"cannot register class"};
            return result;
        }

        auto title = windowConfig.getConfig<std::string>(OMNIX_WINDOW_CONFIG_TITLE);
        auto height = windowConfig.getConfig<int>(OMNIX_WINDOW_CONFIG_HEIGHT);
        auto width = windowConfig.getConfig<int>(OMNIX_WINDOW_CONFIG_WIDTH);
       
        hwnd = CreateWindowEx(
            0,
            wc.lpszClassName,
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

        
        if(graphic_backend==OpenGLBACKEND){
            HGLRC rc = nullptr;
            PIXELFORMATDESCRIPTOR pfd;
            context = std::make_shared<Defaults::OpenGLWinContext>(Defaults::WinContext(hwnd,hdc),rc,pfd);
        }

        Defaults::OmnixWindowSetGraphicContextEvent __wsgc(context,WinCONTEXT);
        omnix.eventBus().publish(&__wsgc);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

        RAWINPUTDEVICE rid[2];
        rid[0].usUsagePage = 0x01;  
        rid[0].usUsage = 0x06;      
        rid[0].dwFlags = 0;         
        rid[0].hwndTarget = hwnd; 

        rid[1].usUsagePage = 0x01;
        rid[1].usUsage = 0x02; 
        rid[1].dwFlags = 0;
        rid[1].hwndTarget = hwnd;

        if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
          result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"cannot register keyboard raw input device! (this will cause input issues)"};
        }
 
        if (!hwnd) {
            result<<OmnixResultContext{OmnixResultContextLevel::TRACE,""};
            result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"hwnd is null!"};
            return result;
        }

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

    }END_RESULT_FUNC

    RESULT_FUNC(update,WindowsWindowBackend,Omnix::Core::Omnix& omnix,OmnixMainPhaseEvent* event){
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                event->OmnixRunState = Core::OmnixState::STOP;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto __wre = Defaults::OmnixWindowRenderEvent{context,WinCONTEXT};

        omnix.eventBus().publish(&__wre);


        if (!hwnd) {
            result<<OmnixResultContext{OmnixResultContextLevel::TRACE,""};
            result<<OmnixResultContext{OmnixResultContextLevel::__ERROR,"hwnd is null!"};
            event->OmnixRunState = Core::OmnixState::STOP;
            return result;
        }
    }END_RESULT_FUNC

    RESULT_FUNC(destroy,WindowsWindowBackend,Omnix::Core::Omnix& omnix){

    }END_RESULT_FUNC
};
#endif  //_WIN32
#endif // OMNIX_WINDOW_BACKEND_H