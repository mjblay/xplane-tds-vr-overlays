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
xpto::MarkerPosition g_anchorPosition;
xpto::MarkerPosition g_bodyOffset;
xpto::MarkerRotation g_rotationOffset;
float g_baseHeadingDegrees = 0.0f;

void Log(const char* message) {
    XPLMDebugString(message);
}

void LogState(const char* label) {
    char buffer[512] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "XPTO: %s marker target=test_marker body_offset x=%.3f y=%.3f z=%.3f rot roll=%.2f pitch=%.2f yaw=%.2f final local x=%.2f y=%.2f z=%.2f pitch=%.2f heading=%.2f roll=%.2f\n",
        label,
        g_bodyOffset.x,
        g_bodyOffset.y,
        g_bodyOffset.z,
        g_rotationOffset.roll,
        g_rotationOffset.pitch,
        g_rotationOffset.yaw,
        g_position.x,
        g_position.y,
        g_position.z,
        g_position.pitch,
        g_position.heading,
        g_position.roll);
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

void RecomputeDrawInfoFromControlState() {
    g_position.structSize = sizeof(g_position);
    g_position.x = g_anchorPosition.x + g_bodyOffset.x;
    g_position.y = g_anchorPosition.y + g_bodyOffset.y;
    g_position.z = g_anchorPosition.z + g_bodyOffset.z;
    g_position.pitch = g_rotationOffset.pitch;
    g_position.heading = g_baseHeadingDegrees + g_rotationOffset.yaw;
    g_position.roll = g_rotationOffset.roll;
}

void InitializePositionNearAircraft() {
    FindPositionDataRefs();

    g_position = {};
    g_bodyOffset = {};
    g_rotationOffset = {};
    g_baseHeadingDegrees = 0.0f;

    if (g_localX == nullptr || g_localY == nullptr || g_localZ == nullptr) {
        Log("XPTO: local position datarefs unavailable; placing marker at local origin fallback.\n");
        g_anchorPosition = {};
        g_positionInitialized = true;
        RecomputeDrawInfoFromControlState();
        LogState("initialized fallback");
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

    g_anchorPosition.x = static_cast<float>(aircraftX + std::sin(headingRadians) * kInitialForwardOffsetMeters);
    g_anchorPosition.y = static_cast<float>(aircraftY + kInitialUpOffsetMeters);
    g_anchorPosition.z = static_cast<float>(aircraftZ - std::cos(headingRadians) * kInitialForwardOffsetMeters);
    g_baseHeadingDegrees = static_cast<float>(aircraftHeadingDegrees);
    g_positionInitialized = true;

    Log("XPTO: marker anchor is 8.0m forward along aircraft true heading and 3.0m above aircraft local position. Body offsets start at zero.\n");
    RecomputeDrawInfoFromControlState();
    LogState("initialized");
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

void ApplyPosition(const char* label) {
    if (g_instance == nullptr) {
        return;
    }

    RecomputeDrawInfoFromControlState();
    float data[1] = {0.0f};
    XPLMInstanceSetPosition(g_instance, &g_position, data);
    LogState(label);
}

}  // namespace

namespace xpto {

void ShowTestMarker() {
    if (!EnsureInstanceCreated()) {
        return;
    }

    ApplyPosition("shown");
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

    g_bodyOffset.x += dxMeters;
    g_bodyOffset.y += dyMeters;
    g_bodyOffset.z += dzMeters;

    ApplyPosition("translated");
}

void RotateTestMarker(MarkerRotationAxis axis, float deltaDegrees) {
    if (!EnsureInstanceCreated()) {
        return;
    }

    if (axis == MarkerRotationAxis::Roll) {
        g_rotationOffset.roll += deltaDegrees;
    } else if (axis == MarkerRotationAxis::Pitch) {
        g_rotationOffset.pitch += deltaDegrees;
    } else {
        g_rotationOffset.yaw += deltaDegrees;
    }

    ApplyPosition("rotated");
}

MarkerState GetTestMarkerState() {
    MarkerState state;
    state.visible = g_instance != nullptr;
    state.hasPosition = g_positionInitialized;
    state.position.x = g_position.x;
    state.position.y = g_position.y;
    state.position.z = g_position.z;
    state.bodyOffset = g_bodyOffset;
    state.rotation = g_rotationOffset;
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