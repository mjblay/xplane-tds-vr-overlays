#include "xpto_window.h"

#include <cstdio>

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"

#include "xpto_object_instance.h"

namespace {

struct WindowGeometry {
    int left = 100;
    int top = 700;
    int right = 620;
    int bottom = 270;
};

struct ButtonBounds {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

enum class ButtonAction {
    None,
    ShowMarker,
    HideMarker,
    XPositive,
    XNegative,
    YPositive,
    YNegative,
    ZPositive,
    ZNegative,
    CycleMoveStep,
    CycleRotStep,
    RollPositive,
    RollNegative,
    PitchPositive,
    PitchNegative,
    YawPositive,
    YawNegative,
};

struct Button {
    ButtonBounds bounds;
    ButtonAction action = ButtonAction::None;
    const char* label = "";
};

constexpr int kMaxButtons = 18;
constexpr float kMoveStepSizes[] = {1.0f, 0.25f, 0.05f, 0.005f, 0.001f};
constexpr float kRotStepSizes[] = {15.0f, 5.0f, 1.0f, 0.1f};

XPLMWindowID g_window = nullptr;
WindowGeometry g_lastDesktopGeometry;
bool g_hasLastDesktopGeometry = false;
bool g_vrActive = false;
Button g_buttons[kMaxButtons];
int g_buttonCount = 0;
int g_moveStepIndex = 0;
int g_rotStepIndex = 0;

void Log(const char* message) {
    XPLMDebugString(message);
}

float CurrentMoveStepMeters() {
    return kMoveStepSizes[g_moveStepIndex];
}

float CurrentRotStepDegrees() {
    return kRotStepSizes[g_rotStepIndex];
}

WindowGeometry DefaultDesktopGeometry() {
    int screenLeft = 0;
    int screenTop = 900;
    int screenRight = 1600;
    int screenBottom = 0;
    XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);

    constexpr int windowWidth = 520;
    constexpr int windowHeight = 430;
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

void AddButton(int left, int top, int width, int height, ButtonAction action, const char* label) {
    if (g_buttonCount >= kMaxButtons) {
        return;
    }

    Button& button = g_buttons[g_buttonCount++];
    button.bounds.left = left;
    button.bounds.top = top;
    button.bounds.right = left + width;
    button.bounds.bottom = top - height;
    button.action = action;
    button.label = label;
}

void BuildButtons(int left, int top) {
    g_buttonCount = 0;

    constexpr int buttonWidth = 104;
    constexpr int buttonHeight = 28;
    constexpr int gap = 10;
    const int row1 = top - 176;
    const int row2 = row1 - buttonHeight - gap;
    const int row3 = row2 - buttonHeight - gap;
    const int row4 = row3 - buttonHeight - gap;
    const int row5 = row4 - buttonHeight - gap;
    const int col1 = left + 16;
    const int col2 = col1 + buttonWidth + gap;
    const int col3 = col2 + buttonWidth + gap;
    const int col4 = col3 + buttonWidth + gap;

    AddButton(col1, row1, buttonWidth, buttonHeight, ButtonAction::ShowMarker, "Show Marker");
    AddButton(col2, row1, buttonWidth, buttonHeight, ButtonAction::HideMarker, "Hide Marker");
    AddButton(col3, row1, buttonWidth, buttonHeight, ButtonAction::CycleMoveStep, "Move Step");
    AddButton(col4, row1, buttonWidth, buttonHeight, ButtonAction::CycleRotStep, "Rot Step");

    AddButton(col1, row2, buttonWidth, buttonHeight, ButtonAction::XNegative, "Right/Stbd");
    AddButton(col2, row2, buttonWidth, buttonHeight, ButtonAction::XPositive, "Left/Port");
    AddButton(col3, row2, buttonWidth, buttonHeight, ButtonAction::YNegative, "Down");
    AddButton(col4, row2, buttonWidth, buttonHeight, ButtonAction::YPositive, "Up");

    AddButton(col1, row3, buttonWidth, buttonHeight, ButtonAction::ZNegative, "Aft");
    AddButton(col2, row3, buttonWidth, buttonHeight, ButtonAction::ZPositive, "Fore");

    AddButton(col1, row4, buttonWidth, buttonHeight, ButtonAction::RollNegative, "Roll -");
    AddButton(col2, row4, buttonWidth, buttonHeight, ButtonAction::RollPositive, "Roll +");
    AddButton(col3, row4, buttonWidth, buttonHeight, ButtonAction::PitchNegative, "Pitch -");
    AddButton(col4, row4, buttonWidth, buttonHeight, ButtonAction::PitchPositive, "Pitch +");

    AddButton(col1, row5, buttonWidth, buttonHeight, ButtonAction::YawNegative, "Yaw -");
    AddButton(col2, row5, buttonWidth, buttonHeight, ButtonAction::YawPositive, "Yaw +");
}

void DrawButton(const Button& button) {
    XPLMDrawTranslucentDarkBox(button.bounds.left, button.bounds.top, button.bounds.right, button.bounds.bottom);

    float white[] = {1.0f, 1.0f, 1.0f};
    char label[64] = {};
    std::snprintf(label, sizeof(label), "%s", button.label);
    XPLMDrawString(white, button.bounds.left + 10, button.bounds.top - 19, label, nullptr, xplmFont_Basic);
}

ButtonAction HitTestButton(int x, int y) {
    for (int i = 0; i < g_buttonCount; ++i) {
        const Button& button = g_buttons[i];
        if (x >= button.bounds.left && x <= button.bounds.right && y <= button.bounds.top && y >= button.bounds.bottom) {
            return button.action;
        }
    }

    return ButtonAction::None;
}

void LogButtonAction(const char* label) {
    char buffer[128] = {};
    std::snprintf(buffer, sizeof(buffer), "XPTO: tuner window button clicked: %s\n", label);
    Log(buffer);
}

void ExecuteButtonAction(ButtonAction action) {
    const float moveStep = CurrentMoveStepMeters();
    const float rotStep = CurrentRotStepDegrees();

    switch (action) {
        case ButtonAction::ShowMarker:
            LogButtonAction("Show Marker");
            xpto::ShowTestMarker();
            break;
        case ButtonAction::HideMarker:
            LogButtonAction("Hide Marker");
            xpto::HideTestMarker();
            break;
        case ButtonAction::XPositive:
            LogButtonAction("Left/Port");
            xpto::NudgeTestMarker(moveStep, 0.0f, 0.0f);
            break;
        case ButtonAction::XNegative:
            LogButtonAction("Right/Stbd");
            xpto::NudgeTestMarker(-moveStep, 0.0f, 0.0f);
            break;
        case ButtonAction::YPositive:
            LogButtonAction("Up");
            xpto::NudgeTestMarker(0.0f, moveStep, 0.0f);
            break;
        case ButtonAction::YNegative:
            LogButtonAction("Down");
            xpto::NudgeTestMarker(0.0f, -moveStep, 0.0f);
            break;
        case ButtonAction::ZPositive:
            LogButtonAction("Fore");
            xpto::NudgeTestMarker(0.0f, 0.0f, moveStep);
            break;
        case ButtonAction::ZNegative:
            LogButtonAction("Aft");
            xpto::NudgeTestMarker(0.0f, 0.0f, -moveStep);
            break;
        case ButtonAction::CycleMoveStep:
            g_moveStepIndex = (g_moveStepIndex + 1) % 5;
            LogButtonAction("Move Step");
            break;
        case ButtonAction::CycleRotStep:
            g_rotStepIndex = (g_rotStepIndex + 1) % 4;
            LogButtonAction("Rot Step");
            break;
        case ButtonAction::RollPositive:
            LogButtonAction("Roll +");
            xpto::RotateTestMarker(xpto::MarkerRotationAxis::Roll, rotStep);
            break;
        case ButtonAction::RollNegative:
            LogButtonAction("Roll -");
            xpto::RotateTestMarker(xpto::MarkerRotationAxis::Roll, -rotStep);
            break;
        case ButtonAction::PitchPositive:
            LogButtonAction("Pitch +");
            xpto::RotateTestMarker(xpto::MarkerRotationAxis::Pitch, rotStep);
            break;
        case ButtonAction::PitchNegative:
            LogButtonAction("Pitch -");
            xpto::RotateTestMarker(xpto::MarkerRotationAxis::Pitch, -rotStep);
            break;
        case ButtonAction::YawPositive:
            LogButtonAction("Yaw +");
            xpto::RotateTestMarker(xpto::MarkerRotationAxis::Yaw, rotStep);
            break;
        case ButtonAction::YawNegative:
            LogButtonAction("Yaw -");
            xpto::RotateTestMarker(xpto::MarkerRotationAxis::Yaw, -rotStep);
            break;
        case ButtonAction::None:
            break;
    }
}

void DrawWindow(XPLMWindowID windowId, void*) {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    XPLMGetWindowGeometry(windowId, &left, &top, &right, &bottom);

    XPLMDrawTranslucentDarkBox(left, top, right, bottom);

    const xpto::MarkerState markerState = xpto::GetTestMarkerState();

    float white[] = {1.0f, 1.0f, 1.0f};
    char title[] = "XPTO Runtime Tuner";
    char subtitle[] = "XPLM instance proof - no overlay movement";
    char desktopMode[] = "Window mode: 2D floating";
    char vrMode[] = "Window mode: VR";
    char* modeText = g_vrActive ? vrMode : desktopMode;
    char markerText[96] = {};
    char moveStepText[64] = {};
    char rotStepText[64] = {};
    char offsetText[128] = {};
    char rotationText[128] = {};
    char positionText[128] = {};

    std::snprintf(markerText, sizeof(markerText), "Marker: %s", markerState.visible ? "shown" : "hidden");
    const float moveStepMeters = CurrentMoveStepMeters();
    if (moveStepMeters < 0.01f) {
        std::snprintf(moveStepText, sizeof(moveStepText), "Move Step: %.3f m", moveStepMeters);
    } else {
        std::snprintf(moveStepText, sizeof(moveStepText), "Move Step: %.2f m", moveStepMeters);
    }
    std::snprintf(rotStepText, sizeof(rotStepText), "Rot Step: %.1f deg", CurrentRotStepDegrees());
    std::snprintf(
        offsetText,
        sizeof(offsetText),
        "Body offset XYZ: %.3f, %.3f, %.3f",
        markerState.bodyOffset.x,
        markerState.bodyOffset.y,
        markerState.bodyOffset.z);
    std::snprintf(
        rotationText,
        sizeof(rotationText),
        "Rot R/P/Y: %.1f, %.1f, %.1f deg",
        markerState.rotation.roll,
        markerState.rotation.pitch,
        markerState.rotation.yaw);
    if (markerState.hasPosition) {
        std::snprintf(
            positionText,
            sizeof(positionText),
            "Final local XYZ: %.2f, %.2f, %.2f",
            markerState.position.x,
            markerState.position.y,
            markerState.position.z);
    } else {
        std::snprintf(positionText, sizeof(positionText), "Final local XYZ: unavailable");
    }

    XPLMDrawString(white, left + 16, top - 28, title, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 50, subtitle, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 72, modeText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 94, markerText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 150, top - 94, moveStepText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 330, top - 94, rotStepText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 116, offsetText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 138, rotationText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 160, positionText, nullptr, xplmFont_Basic);

    BuildButtons(left, top);
    for (int i = 0; i < g_buttonCount; ++i) {
        DrawButton(g_buttons[i]);
    }
}

void HandleKey(XPLMWindowID, char, XPLMKeyFlags, char, void*, int) {
}

int HandleMouseClick(XPLMWindowID, int x, int y, XPLMMouseStatus status, void*) {
    if (status == xplm_MouseDown) {
        const ButtonAction action = HitTestButton(x, y);
        if (action != ButtonAction::None) {
            ExecuteButtonAction(action);
        }
    }

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
    XPLMSetWindowResizingLimits(g_window, 500, 380, 980, 700);
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