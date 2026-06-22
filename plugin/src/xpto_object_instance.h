#pragma once

namespace xpto {

struct ProxyPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ProxyRotation {
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
};

struct ProxySize {
    float width = 0.0f;
    float height = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct ProxyState {
    const char* targetName = "";
    bool objectLoaded = false;
    bool instanceCreated = false;
    bool visible = false;
    bool hasPosition = false;
    ProxyPosition finalLocalPosition;
    ProxyPosition placement;
    ProxyRotation rotation;
    ProxySize size;
    bool calibrated = false;
    ProxyPosition calibrationOffset;
    ProxyRotation calibrationRotationOffset;
    ProxyPosition correctedPlacement;
    ProxyRotation correctedRotation;
    const char* calibrationPath = "";
    const char* lastExportPath = "";
    bool lastExportSucceeded = false;
};

enum class ProxyRotationAxis {
    Roll,
    Pitch,
    Yaw,
};

enum class ProxySizeAxis {
    Width,
    Height,
};

void CycleSelectedProxyTarget();
ProxyState GetSelectedProxyState();
void ShowSelectedProxy();
void HideSelectedProxy();
void ResetSelectedProxy();
void NudgeSelectedProxy(float dxMeters, float dyMeters, float dzMeters);
void RotateSelectedProxy(ProxyRotationAxis axis, float deltaDegrees);
void ResizeSelectedProxy(ProxySizeAxis axis, float deltaMeters);
void CalibrateSelectedProxyHere();
void ClearSelectedProxyCalibration();
void ExportSelectedProxyPlacement();
void DestroyAllProxies();

}  // namespace xpto
