#include "xpto_window.h"

#include <cstdio>

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"

#include "xpto_object_instance.h"

namespace {

struct WindowGeometry {
    int left = 100;
    int top = 760;
    int right = 660;
    int bottom = 210;
};

struct ButtonBounds {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

enum class ButtonAction {
    None,
    CycleTarget,
    ShowProxy,
    HideProxy,
    ExportPlacement,
    ResetProxy,
    CalibrateHere,
    ClearCalibration,
    XPositive,
    XNegative,
    YPositive,
    YNegative,
    ZPositive,
    ZNegative,
    CycleMoveStep,
    CycleRotStep,
    CycleSizeStep,
    RollPositive,
    RollNegative,
    PitchPositive,
    PitchNegative,
    YawPositive,
    YawNegative,
    WidthPositive,
    WidthNegative,
    HeightPositive,
    HeightNegative,
};

struct Button {
    ButtonBounds bounds;
    ButtonAction action = ButtonAction::None;
    const char* label = "";
};

constexpr int kMaxButtons = 32;
constexpr float kMoveStepSizes[] = {1.0f, 0.25f, 0.05f, 0.005f, 0.001f};
constexpr float kRotStepSizes[] = {15.0f, 5.0f, 1.0f, 0.1f};
constexpr float kSizeStepSizes[] = {0.05f, 0.01f, 0.005f, 0.001f};

XPLMWindowID g_window = nullptr;
WindowGeometry g_lastDesktopGeometry;
bool g_hasLastDesktopGeometry = false;
bool g_vrActive = false;
Button g_buttons[kMaxButtons];
int g_buttonCount = 0;
int g_moveStepIndex = 0;
int g_rotStepIndex = 0;
int g_sizeStepIndex = 0;

void Log(const char* message) {
    XPLMDebugString(message);
}

float CurrentMoveStepMeters() {
    return kMoveStepSizes[g_moveStepIndex];
}

float CurrentRotStepDegrees() {
    return kRotStepSizes[g_rotStepIndex];
}

float CurrentSizeStepMeters() {
    return kSizeStepSizes[g_sizeStepIndex];
}

WindowGeometry DefaultDesktopGeometry() {
    int screenLeft = 0;
    int screenTop = 900;
    int screenRight = 1600;
    int screenBottom = 0;
    XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);

    constexpr int windowWidth = 560;
    constexpr int windowHeight = 660;
    constexpr int marginLeft = 120;
    constexpr int marginTop = 100;

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

    constexpr int buttonWidth = 124;
    constexpr int buttonHeight = 28;
    constexpr int gap = 8;
    const int row1 = top - 304;
    const int row2 = row1 - buttonHeight - gap;
    const int row3 = row2 - buttonHeight - gap;
    const int row4 = row3 - buttonHeight - gap;
    const int row5 = row4 - buttonHeight - gap;
    const int row6 = row5 - buttonHeight - gap;
    const int row7 = row6 - buttonHeight - gap;
    const int col1 = left + 16;
    const int col2 = col1 + buttonWidth + gap;
    const int col3 = col2 + buttonWidth + gap;
    const int col4 = col3 + buttonWidth + gap;

    AddButton(col1, row1, buttonWidth, buttonHeight, ButtonAction::CycleTarget, "Target");
    AddButton(col2, row1, buttonWidth, buttonHeight, ButtonAction::ShowProxy, "Show Proxy");
    AddButton(col3, row1, buttonWidth, buttonHeight, ButtonAction::HideProxy, "Hide Proxy");
    AddButton(col4, row1, buttonWidth, buttonHeight, ButtonAction::ExportPlacement, "Export");

    AddButton(col1, row2, buttonWidth, buttonHeight, ButtonAction::CycleMoveStep, "Move Step");
    AddButton(col2, row2, buttonWidth, buttonHeight, ButtonAction::CycleRotStep, "Rot Step");
    AddButton(col3, row2, buttonWidth, buttonHeight, ButtonAction::CycleSizeStep, "Size Step");
    AddButton(col4, row2, buttonWidth, buttonHeight, ButtonAction::ResetProxy, "Reset Proxy");

    AddButton(col1, row3, buttonWidth, buttonHeight, ButtonAction::XNegative, "Right/Stbd");
    AddButton(col2, row3, buttonWidth, buttonHeight, ButtonAction::XPositive, "Left/Port");
    AddButton(col3, row3, buttonWidth, buttonHeight, ButtonAction::YNegative, "Down");
    AddButton(col4, row3, buttonWidth, buttonHeight, ButtonAction::YPositive, "Up");

    AddButton(col1, row4, buttonWidth, buttonHeight, ButtonAction::ZNegative, "Aft");
    AddButton(col2, row4, buttonWidth, buttonHeight, ButtonAction::ZPositive, "Fore");
    AddButton(col3, row4, buttonWidth, buttonHeight, ButtonAction::WidthNegative, "Width -");
    AddButton(col4, row4, buttonWidth, buttonHeight, ButtonAction::WidthPositive, "Width +");

    AddButton(col1, row5, buttonWidth, buttonHeight, ButtonAction::HeightNegative, "Height -");
    AddButton(col2, row5, buttonWidth, buttonHeight, ButtonAction::HeightPositive, "Height +");
    AddButton(col3, row5, buttonWidth, buttonHeight, ButtonAction::RollNegative, "Roll -");
    AddButton(col4, row5, buttonWidth, buttonHeight, ButtonAction::RollPositive, "Roll +");

    AddButton(col1, row6, buttonWidth, buttonHeight, ButtonAction::PitchNegative, "Pitch -");
    AddButton(col2, row6, buttonWidth, buttonHeight, ButtonAction::PitchPositive, "Pitch +");
    AddButton(col3, row6, buttonWidth, buttonHeight, ButtonAction::YawNegative, "Yaw -");
    AddButton(col4, row6, buttonWidth, buttonHeight, ButtonAction::YawPositive, "Yaw +");

    AddButton(col1, row7, buttonWidth, buttonHeight, ButtonAction::CalibrateHere, "Calibrate Here");
    AddButton(col2, row7, buttonWidth, buttonHeight, ButtonAction::ClearCalibration, "Clear Cal");
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
    const float sizeStep = CurrentSizeStepMeters();

    switch (action) {
        case ButtonAction::CycleTarget:
            LogButtonAction("Target");
            xpto::CycleSelectedProxyTarget();
            break;
        case ButtonAction::ShowProxy:
            LogButtonAction("Show Proxy");
            xpto::ShowSelectedProxy();
            break;
        case ButtonAction::HideProxy:
            LogButtonAction("Hide Proxy");
            xpto::HideSelectedProxy();
            break;
        case ButtonAction::ExportPlacement:
            LogButtonAction("Export");
            xpto::ExportSelectedProxyPlacement();
            break;
        case ButtonAction::ResetProxy:
            LogButtonAction("Reset Proxy");
            xpto::ResetSelectedProxy();
            break;
        case ButtonAction::CalibrateHere:
            LogButtonAction("Calibrate Here");
            xpto::CalibrateSelectedProxyHere();
            break;
        case ButtonAction::ClearCalibration:
            LogButtonAction("Clear Cal");
            xpto::ClearSelectedProxyCalibration();
            break;
        case ButtonAction::XPositive:
            LogButtonAction("Left/Port");
            xpto::NudgeSelectedProxy(moveStep, 0.0f, 0.0f);
            break;
        case ButtonAction::XNegative:
            LogButtonAction("Right/Stbd");
            xpto::NudgeSelectedProxy(-moveStep, 0.0f, 0.0f);
            break;
        case ButtonAction::YPositive:
            LogButtonAction("Up");
            xpto::NudgeSelectedProxy(0.0f, moveStep, 0.0f);
            break;
        case ButtonAction::YNegative:
            LogButtonAction("Down");
            xpto::NudgeSelectedProxy(0.0f, -moveStep, 0.0f);
            break;
        case ButtonAction::ZPositive:
            LogButtonAction("Fore");
            xpto::NudgeSelectedProxy(0.0f, 0.0f, moveStep);
            break;
        case ButtonAction::ZNegative:
            LogButtonAction("Aft");
            xpto::NudgeSelectedProxy(0.0f, 0.0f, -moveStep);
            break;
        case ButtonAction::CycleMoveStep:
            g_moveStepIndex = (g_moveStepIndex + 1) % 5;
            LogButtonAction("Move Step");
            break;
        case ButtonAction::CycleRotStep:
            g_rotStepIndex = (g_rotStepIndex + 1) % 4;
            LogButtonAction("Rot Step");
            break;
        case ButtonAction::CycleSizeStep:
            g_sizeStepIndex = (g_sizeStepIndex + 1) % 4;
            LogButtonAction("Size Step");
            break;
        case ButtonAction::RollPositive:
            LogButtonAction("Roll +");
            xpto::RotateSelectedProxy(xpto::ProxyRotationAxis::Roll, rotStep);
            break;
        case ButtonAction::RollNegative:
            LogButtonAction("Roll -");
            xpto::RotateSelectedProxy(xpto::ProxyRotationAxis::Roll, -rotStep);
            break;
        case ButtonAction::PitchPositive:
            LogButtonAction("Pitch +");
            xpto::RotateSelectedProxy(xpto::ProxyRotationAxis::Pitch, rotStep);
            break;
        case ButtonAction::PitchNegative:
            LogButtonAction("Pitch -");
            xpto::RotateSelectedProxy(xpto::ProxyRotationAxis::Pitch, -rotStep);
            break;
        case ButtonAction::YawPositive:
            LogButtonAction("Yaw +");
            xpto::RotateSelectedProxy(xpto::ProxyRotationAxis::Yaw, rotStep);
            break;
        case ButtonAction::YawNegative:
            LogButtonAction("Yaw -");
            xpto::RotateSelectedProxy(xpto::ProxyRotationAxis::Yaw, -rotStep);
            break;
        case ButtonAction::WidthPositive:
            LogButtonAction("Width +");
            xpto::ResizeSelectedProxy(xpto::ProxySizeAxis::Width, sizeStep);
            break;
        case ButtonAction::WidthNegative:
            LogButtonAction("Width -");
            xpto::ResizeSelectedProxy(xpto::ProxySizeAxis::Width, -sizeStep);
            break;
        case ButtonAction::HeightPositive:
            LogButtonAction("Height +");
            xpto::ResizeSelectedProxy(xpto::ProxySizeAxis::Height, sizeStep);
            break;
        case ButtonAction::HeightNegative:
            LogButtonAction("Height -");
            xpto::ResizeSelectedProxy(xpto::ProxySizeAxis::Height, -sizeStep);
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

    const xpto::ProxyState proxyState = xpto::GetSelectedProxyState();

    float white[] = {1.0f, 1.0f, 1.0f};
    char title[] = "XPTO Runtime Tuner";
    char subtitle[] = "Developer ACF placement proxy/export";
    char desktopMode[] = "Window mode: 2D floating";
    char vrMode[] = "Window mode: VR";
    char* modeText = g_vrActive ? vrMode : desktopMode;
    char targetText[128] = {};
    char stateText[128] = {};
    char stepText[160] = {};
    char placementText[160] = {};
    char rotationText[160] = {};
    char sizeText[160] = {};
    char calibrationText[180] = {};
    char correctedText[200] = {};
    char finalText[160] = {};
    char exportText[220] = {};

    std::snprintf(targetText, sizeof(targetText), "Target: %s", proxyState.targetName);
    std::snprintf(
        stateText,
        sizeof(stateText),
        "Proxy: %s  Loaded: %s  Instance: %s",
        proxyState.visible ? "shown" : "hidden",
        proxyState.objectLoaded ? "yes" : "no",
        proxyState.instanceCreated ? "yes" : "no");
    std::snprintf(
        stepText,
        sizeof(stepText),
        "Move %.3f m   Rot %.1f deg   Size %.3f m",
        CurrentMoveStepMeters(),
        CurrentRotStepDegrees(),
        CurrentSizeStepMeters());
    std::snprintf(
        placementText,
        sizeof(placementText),
        "ACF candidate XYZ: %.3f, %.3f, %.3f",
        proxyState.placement.x,
        proxyState.placement.y,
        proxyState.placement.z);
    std::snprintf(
        rotationText,
        sizeof(rotationText),
        "ACF pitch/yaw/roll: %.2f, %.2f, %.2f deg",
        proxyState.rotation.pitch,
        proxyState.rotation.yaw,
        proxyState.rotation.roll);
    std::snprintf(
        sizeText,
        sizeof(sizeText),
        "Proxy size W/H: %.4f, %.4f m  scale %.3f, %.3f",
        proxyState.size.width,
        proxyState.size.height,
        proxyState.size.scaleX,
        proxyState.size.scaleY);
    std::snprintf(
        calibrationText,
        sizeof(calibrationText),
        "Calibration: %s  Offset XYZ: %.3f, %.3f, %.3f",
        proxyState.calibrated ? "yes" : "no",
        proxyState.calibrationOffset.x,
        proxyState.calibrationOffset.y,
        proxyState.calibrationOffset.z);
    std::snprintf(
        correctedText,
        sizeof(correctedText),
        "Corrected ACF XYZ: %.3f, %.3f, %.3f  P/Y/R: %.2f, %.2f, %.2f",
        proxyState.correctedPlacement.x,
        proxyState.correctedPlacement.y,
        proxyState.correctedPlacement.z,
        proxyState.correctedRotation.pitch,
        proxyState.correctedRotation.yaw,
        proxyState.correctedRotation.roll);
    if (proxyState.hasPosition) {
        std::snprintf(
            finalText,
            sizeof(finalText),
            "Debug final local XYZ: %.2f, %.2f, %.2f",
            proxyState.finalLocalPosition.x,
            proxyState.finalLocalPosition.y,
            proxyState.finalLocalPosition.z);
    } else {
        std::snprintf(finalText, sizeof(finalText), "Debug final local XYZ: unavailable");
    }
    if (proxyState.lastExportPath[0] != '\0') {
        std::snprintf(
            exportText,
            sizeof(exportText),
            "Last export: %s",
            proxyState.lastExportSucceeded ? proxyState.lastExportPath : "failed");
    } else {
        std::snprintf(exportText, sizeof(exportText), "Last export: none");
    }

    XPLMDrawString(white, left + 16, top - 28, title, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 50, subtitle, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 72, modeText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 94, targetText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 116, stateText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 138, stepText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 160, placementText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 182, rotationText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 204, sizeText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 226, calibrationText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 248, correctedText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, top - 270, finalText, nullptr, xplmFont_Basic);
    XPLMDrawString(white, left + 16, bottom + 18, exportText, nullptr, xplmFont_Basic);

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
    XPLMSetWindowResizingLimits(g_window, 540, 620, 1100, 900);
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

