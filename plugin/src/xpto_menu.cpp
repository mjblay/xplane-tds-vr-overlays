#include "xpto_menu.h"

#include "XPLMMenus.h"

#include "xpto_window.h"

namespace {

XPLMMenuID g_menu = nullptr;

void MenuHandler(void*, void*) {
    xpto::ToggleRuntimeWindow();
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

    XPLMAppendMenuItem(g_menu, "Show Runtime Tuner", nullptr, 0);
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
