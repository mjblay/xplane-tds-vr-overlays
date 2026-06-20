#pragma once

namespace xpto {

enum class MarkerAxis {
    X,
    Y,
    Z,
};

void ShowTestMarker();
void HideTestMarker();
void NudgeTestMarker(MarkerAxis axis, float deltaMeters);
void DestroyTestMarker();

}  // namespace xpto