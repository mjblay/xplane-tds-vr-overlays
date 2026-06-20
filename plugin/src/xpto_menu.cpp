#include "xpto_menu.h"

#include <cstdint>

#include "XPLMMenus.h"

#include "xpto_object_instance.h"
#include "xpto_window.h"

namespace {

constexpr float kNudgeMeters = 1.0f;

XPLMMenuID g_menu = nullptr;

enum class MenuAction : intptr_t {
    ShowRuntimeTuner = 1,
    ShowTestMarker,
    HideTestMarker,
    NudgeXPositive,
    NudgeXNegative,
    NudgeYPositive,
    NudgeYNegative,
    NudgeZPositive,
    NudgeZNegative,
};

void AppendActionItem(const char* label, MenuAction action) {
    XPLMAppendMenuItem(g_menu, label, reinterpret_cast<void*>(static_cast<intptr_t>(action)), 0);
}

void MenuHandler(void*, void* itemRef) {
    const auto action = static_cast<MenuAction>(reinterpret_cast<intptr_t>(itemRef));

    switch (action) {
        case MenuAction::ShowRuntimeTuner:
            xpto::ToggleRuntimeWindow();
            break;
        case MenuAction::ShowTestMarker:
            xpto::ShowTestMarker();
            break;
        case MenuAction::HideTestMarker:
            xpto::HideTestMarker();
            break;
        case MenuAction::NudgeXPositive:
            xpto::NudgeTestMarker(xpto::MarkerAxis::X, kNudgeMeters);
            break;
        case MenuAction::NudgeXNegative:
            xpto::NudgeTestMarker(xpto::MarkerAxis::X, -kNudgeMeters);
            break;
        case MenuAction::NudgeYPositive:
            xpto::NudgeTestMarker(xpto::MarkerAxis::Y, kNudgeMeters);
            break;
        case MenuAction::NudgeYNegative:
            xpto::NudgeTestMarker(xpto::MarkerAxis::Y, -kNudgeMeters);
            break;
        case MenuAction::NudgeZPositive:
            xpto::NudgeTestMarker(xpto::MarkerAxis::Z, kNudgeMeters);
            break;
        case MenuAction::NudgeZNegative:
            xpto::NudgeTestMarker(xpto::MarkerAxis::Z, -kNudgeMeters);
            break;
    }
}

}  // namespace

namespace xpto {

bool CreateMenu() {
    if (g_menu != nullptr) {
        return true;
    }

    XPLMMenuID pluginsMenu = XPLMFindPluginsMenu();
    if (pluginsMenu == nullptr) {
        return false;
    }

    const int menuItem = XPLMAppendMenuItem(pluginsMenu, "XPTO", nullptr, 0);
    g_menu = XPLMCreateMenu("XPTO", pluginsMenu, menuItem, MenuHandler, nullptr);
    if (g_menu == nullptr) {
        return false;
    }

    AppendActionItem("Show Runtime Tuner", MenuAction::ShowRuntimeTuner);
    XPLMAppendMenuSeparator(g_menu);
    AppendActionItem("Show Test Marker", MenuAction::ShowTestMarker);
    AppendActionItem("Hide Test Marker", MenuAction::HideTestMarker);
    AppendActionItem("Nudge Test Marker +X", MenuAction::NudgeXPositive);
    AppendActionItem("Nudge Test Marker -X", MenuAction::NudgeXNegative);
    AppendActionItem("Nudge Test Marker +Y", MenuAction::NudgeYPositive);
    AppendActionItem("Nudge Test Marker -Y", MenuAction::NudgeYNegative);
    AppendActionItem("Nudge Test Marker +Z", MenuAction::NudgeZPositive);
    AppendActionItem("Nudge Test Marker -Z", MenuAction::NudgeZNegative);
    return true;
}

void DestroyMenu() {
    if (g_menu == nullptr) {
        return;
    }

    XPLMDestroyMenu(g_menu);
    g_menu = nullptr;
}

}  // namespace xpto