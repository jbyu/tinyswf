#include "main.h"
#include "../Classes/AppDelegate.h"
#include "CCEGLView.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
//#include <vld.h>
#endif//_DEBUG

USING_NS_CC;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // create the application instance
    AppDelegate app;
    EGLView* eglView = EGLView::getInstance();
    eglView->setViewName("HelloCpp");
    eglView->setFrameSize(640, 960);
    // The resolution of ipad3 is very large. In general, PC's resolution is smaller than it.
    // So we need to invoke 'setFrameZoomFactor'(only valid on desktop(win32, mac, linux)) to make the window smaller.
    //eglView->setFrameZoomFactor(0.4f);
    return Application::getInstance()->run();
}
