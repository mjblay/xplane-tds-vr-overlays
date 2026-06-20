#pragma once

namespace xpto {

struct MarkerPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct MarkerState {
    bool visible = false;
    bool hasPosition = false;
    MarkerPosition position;
};

enum class MarkerAxis {
    X,
    Y,
    Z,
};

void ShowTestMarker();
void HideTestMarker();
void NudgeTestMarker(MarkerAxis axis, float deltaMeters);
void NudgeTestMarker(float dxMeters, float dyMeters, float dzMeters);
MarkerState GetTestMarkerState();
void DestroyTestMarker();

}  // namespace xpto