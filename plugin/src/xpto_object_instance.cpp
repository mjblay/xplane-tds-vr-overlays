#include "xpto_object_instance.h"

#include <cmath>
#include <cstdio>

#include "XPLMDataAccess.h"
#include "XPLMInstance.h"
#include "XPLMScenery.h"
#include "XPLMUtilities.h"

namespace {

constexpr const char* kMarkerObjectPath = "Resources/plugins/XPTO/assets/xpto_test_marker.obj";
constexpr float kInitialForwardOffsetMeters = 8.0f;
constexpr float kInitialUpOffsetMeters = 3.0f;
constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;

XPLMObjectRef g_object = nullptr;
XPLMInstanceRef g_instance = nullptr;
XPLMDrawInfo_t g_position = {};
XPLMDataRef g_localX = nullptr;
XPLMDataRef g_localY = nullptr;
XPLMDataRef g_localZ = nullptr;
XPLMDataRef g_truePsi = nullptr;
bool g_positionInitialized = false;

void Log(const char* message) {
    XPLMDebugString(message);
}

void LogPosition(const char* label) {
    char buffer[256] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "XPTO: %s marker local position x=%.2f y=%.2f z=%.2f\n",
        label,
        g_position.x,
        g_position.y,
        g_position.z);
    Log(buffer);
}

void FindPositionDataRefs() {
    if (g_localX != nullptr && g_localY != nullptr && g_localZ != nullptr && g_truePsi != nullptr) {
        return;
    }

    g_localX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    g_localY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    g_localZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    g_truePsi = XPLMFindDataRef("sim/flightmodel/position/true_psi");
}

void InitializePositionNearAircraft() {
    FindPositionDataRefs();

    g_position = {};
    g_position.structSize = sizeof(g_position);
    g_position.pitch = 0.0f;
    g_position.heading = 0.0f;
    g_position.roll = 0.0f;

    if (g_localX == nullptr || g_localY == nullptr || g_localZ == nullptr) {
        Log("XPTO: local position datarefs unavailable; placing marker at local origin fallback.\n");
        g_position.x = 0.0f;
        g_position.y = 0.0f;
        g_position.z = 0.0f;
        g_positionInitialized = true;
        LogPosition("initialized fallback");
        return;
    }

    const double aircraftX = XPLMGetDatad(g_localX);
    const double aircraftY = XPLMGetDatad(g_localY);
    const double aircraftZ = XPLMGetDatad(g_localZ);
    const double aircraftHeadingDegrees = g_truePsi != nullptr ? XPLMGetDataf(g_truePsi) : 0.0;
    const double headingRadians = aircraftHeadingDegrees * kDegreesToRadians;

    char aircraftBuffer[256] = {};
    std::snprintf(
        aircraftBuffer,
        sizeof(aircraftBuffer),
        "XPTO: aircraft local position x=%.2f y=%.2f z=%.2f true_psi=%.1f\n",
        aircraftX,
        aircraftY,
        aircraftZ,
        aircraftHeadingDegrees);
    Log(aircraftBuffer);

    g_position.x = static_cast<float>(aircraftX + std::sin(headingRadians) * kInitialForwardOffsetMeters);
    g_position.y = static_cast<float>(aircraftY + kInitialUpOffsetMeters);
    g_position.z = static_cast<float>(aircraftZ - std::cos(headingRadians) * kInitialForwardOffsetMeters);
    g_position.heading = static_cast<float>(aircraftHeadingDegrees);
    g_positionInitialized = true;

    Log("XPTO: marker initial offset is 8.0m forward along aircraft true heading and 3.0m above aircraft local position.\n");
    LogPosition("initialized");
}

bool EnsureObjectLoaded() {
    if (g_object != nullptr) {
        return true;
    }

    Log("XPTO: loading test marker OBJ path: Resources/plugins/XPTO/assets/xpto_test_marker.obj\n");
    g_object = XPLMLoadObject(kMarkerObjectPath);

    if (g_object == nullptr) {
        Log("XPTO: failed to load test marker OBJ. Confirm XPTO/assets/xpto_test_marker.obj is installed.\n");
        return false;
    }

    Log("XPTO: test marker OBJ loaded successfully.\n");
    return true;
}

bool EnsureInstanceCreated() {
    if (g_instance != nullptr) {
        return true;
    }

    if (!EnsureObjectLoaded()) {
        return false;
    }

    if (!g_positionInitialized) {
        InitializePositionNearAircraft();
    }

    const char* datarefs[] = {nullptr};
    g_instance = XPLMCreateInstance(g_object, datarefs);

    if (g_instance == nullptr) {
        Log("XPTO: failed to create test marker instance.\n");
        return false;
    }

    Log("XPTO: test marker instance created.\n");
    return true;
}

void ApplyPosition() {
    if (g_instance == nullptr) {
        return;
    }

    float data[1] = {0.0f};
    XPLMInstanceSetPosition(g_instance, &g_position, data);
}

}  // namespace

namespace xpto {

void ShowTestMarker() {
    if (!EnsureInstanceCreated()) {
        return;
    }

    ApplyPosition();
    LogPosition("shown");
}

void HideTestMarker() {
    if (g_instance == nullptr) {
        Log("XPTO: hide test marker requested, but no instance exists.\n");
        return;
    }

    XPLMDestroyInstance(g_instance);
    g_instance = nullptr;
    Log("XPTO: test marker instance destroyed/hidden.\n");
}

void NudgeTestMarker(MarkerAxis axis, float deltaMeters) {
    if (axis == MarkerAxis::X) {
        NudgeTestMarker(deltaMeters, 0.0f, 0.0f);
    } else if (axis == MarkerAxis::Y) {
        NudgeTestMarker(0.0f, deltaMeters, 0.0f);
    } else {
        NudgeTestMarker(0.0f, 0.0f, deltaMeters);
    }
}

void NudgeTestMarker(float dxMeters, float dyMeters, float dzMeters) {
    if (!EnsureInstanceCreated()) {
        return;
    }

    g_position.x += dxMeters;
    g_position.y += dyMeters;
    g_position.z += dzMeters;

    ApplyPosition();
    LogPosition("nudged");
}

MarkerState GetTestMarkerState() {
    MarkerState state;
    state.visible = g_instance != nullptr;
    state.hasPosition = g_positionInitialized;
    state.position.x = g_position.x;
    state.position.y = g_position.y;
    state.position.z = g_position.z;
    return state;
}

void DestroyTestMarker() {
    if (g_instance != nullptr) {
        XPLMDestroyInstance(g_instance);
        g_instance = nullptr;
        Log("XPTO: test marker instance destroyed during plugin cleanup.\n");
    }

    if (g_object != nullptr) {
        XPLMUnloadObject(g_object);
        g_object = nullptr;
        Log("XPTO: test marker OBJ unloaded during plugin cleanup.\n");
    }

    g_positionInitialized = false;
}

}  // namespace xpto