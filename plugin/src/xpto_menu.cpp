#include "xpto_menu.h"

#include <cstdint>

#include "XPLMMenus.h"

#include "xpto_object_instance.h"
#include "xpto_window.h"

namespace {

XPLMMenuID g_menu = nullptr;

enum class MenuAction : intptr_t {
    ShowRuntimeTuner = 1,
    ShowSelectedProxy,
    HideSelectedProxy,
    ExportPlacement,
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
        case MenuAction::ShowSelectedProxy:
            xpto::ShowSelectedProxy();
            break;
        case MenuAction::HideSelectedProxy:
            xpto::HideSelectedProxy();
            break;
        case MenuAction::ExportPlacement:
            xpto::ExportSelectedProxyPlacement();
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
    AppendActionItem("Show Selected Proxy", MenuAction::ShowSelectedProxy);
    AppendActionItem("Hide Selected Proxy", MenuAction::HideSelectedProxy);
    AppendActionItem("Export Placement", MenuAction::ExportPlacement);
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
