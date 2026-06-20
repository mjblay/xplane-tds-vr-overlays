#include "xpto_window.h"

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"

namespace {

XPLMWindowID g_window = nullptr;

void DrawWindow(XPLMWindowID windowId, void*) {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    XPLMGetWindowGeometry(windowId, &left, &top, &right, &bottom);

    XPLMDrawTranslucentDarkBox(left, top, right, bottom);

    float white[] = {1.0f, 1.0f, 1.0f};
    char title[] = "XPTO runtime skeleton";
    char subtitle[] = "No overlay movement implemented";

    XPLMDrawString(white, left + 16, top - 28, title, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 50, subtitle, nullptr, xplmFont_Basic);
}

void HandleKey(XPLMWindowID, char, XPLMKeyFlags, char, void*, int) {
}

int HandleMouseClick(XPLMWindowID, int, int, XPLMMouseStatus, void*) {
    return 1;
}

XPLMCursorStatus HandleCursor(XPLMWindowID, int, int, void*) {
    return xplm_CursorDefault;
}

int HandleMouseWheel(XPLMWindowID, int, int, int, int, void*) {
    return 1;
}

void CreateRuntimeWindow() {
    if (g_window != nullptr) {
        return;
    }

    XPLMCreateWindow_t params = {};
    params.structSize = sizeof(params);
    params.left = 100;
    params.top = 700;
    params.right = 460;
    params.bottom = 580;
    params.visible = 1;
    params.drawWindowFunc = DrawWindow;
    params.handleMouseClickFunc = HandleMouseClick;
    params.handleKeyFunc = HandleKey;
    params.handleCursorFunc = HandleCursor;
    params.handleMouseWheelFunc = HandleMouseWheel;
    params.refcon = nullptr;
    params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
    params.layer = xplm_WindowLayerFloatingWindows;
    params.handleRightClickFunc = HandleMouseClick;

    g_window = XPLMCreateWindowEx(&params);
    if (g_window == nullptr) {
        return;
    }

    XPLMSetWindowTitle(g_window, "XPTO Runtime Tuner");
    XPLMSetWindowPositioningMode(g_window, xplm_WindowPositionFree, -1);
    XPLMSetWindowGravity(g_window, 0.0f, 1.0f, 0.0f, 1.0f);
    XPLMSetWindowResizingLimits(g_window, 320, 100, 800, 400);
}

}  // namespace

namespace xpto {

void ToggleRuntimeWindow() {
    if (g_window == nullptr) {
        CreateRuntimeWindow();
        return;
    }

    DestroyRuntimeWindow();
}

void DestroyRuntimeWindow() {
    if (g_window == nullptr) {
        return;
    }

    XPLMDestroyWindow(g_window);
    g_window = nullptr;
}

}  // namespace xpto
