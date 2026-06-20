#include "xpto_plugin.h"

#include <cstring>

#include "XPLMPlugin.h"

#include "xpto_menu.h"
#include "xpto_object_instance.h"
#include "xpto_window.h"

namespace {

void CopyPluginString(char* destination, const char* source) {
    std::strncpy(destination, source, 255);
    destination[255] = '\0';
}

}  // namespace

extern "C" {

PLUGIN_API int XPluginStart(char* outName, char* outSignature, char* outDescription) {
    CopyPluginString(outName, xpto::kPluginName);
    CopyPluginString(outSignature, xpto::kPluginSignature);
    CopyPluginString(outDescription, xpto::kPluginDescription);

    return xpto::CreateMenu() ? 1 : 0;
}

PLUGIN_API void XPluginStop() {
    xpto::DestroyTestMarker();
    xpto::DestroyRuntimeWindow();
    xpto::DestroyMenu();
}

PLUGIN_API int XPluginEnable() {
    return 1;
}

PLUGIN_API void XPluginDisable() {
    xpto::HideTestMarker();
    xpto::DestroyRuntimeWindow();
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int message, void*) {
    if (message == XPLM_MSG_ENTERED_VR) {
        xpto::SetRuntimeWindowVrActive(true);
    } else if (message == XPLM_MSG_EXITING_VR) {
        xpto::SetRuntimeWindowVrActive(false);
    }
}

}  // extern "C"