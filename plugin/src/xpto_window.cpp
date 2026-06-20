#include "xpto_window.h"

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"

namespace {

struct WindowGeometry {
    int left = 100;
    int top = 700;
    int right = 480;
    int bottom = 580;
};

XPLMWindowID g_window = nullptr;
WindowGeometry g_lastDesktopGeometry;
bool g_hasLastDesktopGeometry = false;
bool g_vrActive = false;

WindowGeometry DefaultDesktopGeometry() {
    int screenLeft = 0;
    int screenTop = 900;
    int screenRight = 1600;
    int screenBottom = 0;
    XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);

    constexpr int windowWidth = 420;
    constexpr int windowHeight = 140;
    constexpr int marginLeft = 120;
    constexpr int marginTop = 120;

    WindowGeometry geometry;
    geometry.left = screenLeft + marginLeft;
    geometry.top = screenTop - marginTop;
    geometry.right = geometry.left + windowWidth;
    geometry.bottom = geometry.top - windowHeight;

    if (geometry.right > screenRight) {
        geometry.right = screenRight - 40;
        geometry.left = geometry.right - windowWidth;
    }

    if (geometry.bottom < screenBottom) {
        geometry.bottom = screenBottom + 40;
        geometry.top = geometry.bottom + windowHeight;
    }

    return geometry;
}

WindowGeometry InitialDesktopGeometry() {
    if (g_hasLastDesktopGeometry) {
        return g_lastDesktopGeometry;
    }

    return DefaultDesktopGeometry();
}

void StoreCurrentDesktopGeometry() {
    if (g_window == nullptr || XPLMWindowIsInVR(g_window)) {
        return;
    }

    WindowGeometry geometry;
    XPLMGetWindowGeometry(g_window, &geometry.left, &geometry.top, &geometry.right, &geometry.bottom);

    if (geometry.right <= geometry.left || geometry.top <= geometry.bottom) {
        return;
    }

    g_lastDesktopGeometry = geometry;
    g_hasLastDesktopGeometry = true;
}

void ApplyDesktopGeometry() {
    if (g_window == nullptr || !g_hasLastDesktopGeometry) {
        return;
    }

    XPLMSetWindowGeometry(
        g_window,
        g_lastDesktopGeometry.left,
        g_lastDesktopGeometry.top,
        g_lastDesktopGeometry.right,
        g_lastDesktopGeometry.bottom);
}

void ApplyCurrentPositioningMode() {
    if (g_window == nullptr) {
        return;
    }

    if (g_vrActive) {
        StoreCurrentDesktopGeometry();
        XPLMSetWindowPositioningMode(g_window, xplm_WindowVR, -1);
        return;
    }

    XPLMSetWindowPositioningMode(g_window, xplm_WindowPositionFree, -1);
    XPLMSetWindowGravity(g_window, 0.0f, 1.0f, 0.0f, 1.0f);
    ApplyDesktopGeometry();
}

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
    char desktopMode[] = "Window mode: 2D floating";
    char vrMode[] = "Window mode: VR";
    char* modeText = g_vrActive ? vrMode : desktopMode;

    XPLMDrawString(white, left + 16, top - 28, title, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 50, subtitle, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 72, modeText, nullptr, xplmFont_Basic);
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

    const WindowGeometry geometry = InitialDesktopGeometry();

    XPLMCreateWindow_t params = {};
    params.structSize = sizeof(params);
    params.left = geometry.left;
    params.top = geometry.top;
    params.right = geometry.right;
    params.bottom = geometry.bottom;
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
    XPLMSetWindowResizingLimits(g_window, 320, 100, 800, 400);
    ApplyCurrentPositioningMode();
    StoreCurrentDesktopGeometry();
}

}  // namespace

namespace xpto {

void ToggleRuntimeWindow() {
    if (g_window == nullptr) {
        CreateRuntimeWindow();
        return;
    }

    const int isVisible = XPLMGetWindowIsVisible(g_window);

    if (isVisible) {
        StoreCurrentDesktopGeometry();
        XPLMSetWindowIsVisible(g_window, 0);
        return;
    }

    ApplyCurrentPositioningMode();
    XPLMSetWindowIsVisible(g_window, 1);
}

void SetRuntimeWindowVrActive(bool active) {
    g_vrActive = active;

    if (g_window == nullptr) {
        return;
    }

    ApplyCurrentPositioningMode();
}

void DestroyRuntimeWindow() {
    if (g_window == nullptr) {
        return;
    }

    StoreCurrentDesktopGeometry();
    XPLMDestroyWindow(g_window);
    g_window = nullptr;
}

}  // namespace xpto