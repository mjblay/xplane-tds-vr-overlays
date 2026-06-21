#pragma once

namespace xpto {

struct MarkerPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct MarkerRotation {
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
};

struct MarkerState {
    bool visible = false;
    bool hasPosition = false;
    MarkerPosition position;
    MarkerPosition bodyOffset;
    MarkerRotation rotation;
};

enum class MarkerAxis {
    X,
    Y,
    Z,
};

enum class MarkerRotationAxis {
    Roll,
    Pitch,
    Yaw,
};

void ShowTestMarker();
void HideTestMarker();
void NudgeTestMarker(MarkerAxis axis, float deltaMeters);
void NudgeTestMarker(float dxMeters, float dyMeters, float dzMeters);
void RotateTestMarker(MarkerRotationAxis axis, float deltaDegrees);
MarkerState GetTestMarkerState();
void DestroyTestMarker();

}  // namespace xpto