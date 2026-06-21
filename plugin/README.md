# XPTO Runtime Plugin Skeleton

This folder contains the minimal native X-Plane plugin skeleton for the XPTO runtime overlay tuner.

The skeleton only:

- registers the XPTO plugin lifecycle callbacks
- adds an `XPTO` submenu under X-Plane's Plugins menu
- adds `Show Runtime Tuner`
- adds test marker instance proof menu actions
- toggles a basic modern XPLM window titled `XPTO Runtime Tuner`
- draws clickable translation and rotation controls inside the runtime tuner window
- uses 2D floating window positioning outside VR and `xplm_WindowVR` while VR is active
- loads an original project-created test OBJ and moves it as an XPLM instance

It does not load XPTO/TDS overlay OBJ files, move overlays, edit aircraft files, or modify installer behavior.

## Requirements

- X-Plane SDK extracted outside this repository
- CMake 3.20 or newer
- Windows: Visual Studio 2022 Build Tools or Visual Studio with Desktop development with C++

Set `XPLANE_SDK_ROOT` to the extracted SDK root containing:

```text
CHeaders/
Libraries/
```

For example:

```text
D:\xplane-dev\sdk\XPSDK\SDK
```

The Windows build expects:

```text
%XPLANE_SDK_ROOT%\CHeaders\XPLM
%XPLANE_SDK_ROOT%\CHeaders\Widgets
%XPLANE_SDK_ROOT%\Libraries\Win\XPLM_64.lib
```

`XPWidgets_64.lib` is not linked because this skeleton uses modern XPLM windows, not XPWidgets.

## Build

From the repository root:

```powershell
cmake -S plugin -B build\plugin -DXPLANE_SDK_ROOT="D:\xplane-dev\sdk\XPSDK\SDK"
cmake --build build\plugin --config Release
```

The Windows build stages a copyable plugin layout:

```text
build\plugin\XPTO\win_x64\XPTO.xpl
build\plugin\XPTO\assets\xpto_test_marker.obj
```

## Manual Install

Copy the staged `XPTO` folder to:

```text
X-Plane 12\Resources\plugins\XPTO
```

Expected installed layout:

```text
X-Plane 12\Resources\plugins\XPTO\win_x64\XPTO.xpl
X-Plane 12\Resources\plugins\XPTO\assets\xpto_test_marker.obj
```

## Runtime Tuner Window Validation

Start X-Plane in 2D and open:

```text
Plugins > XPTO > Show Runtime Tuner
```

The menu item toggles the `XPTO Runtime Tuner` window. In 2D, the window uses normal floating positioning and shows marker status, move step, rotation step, body offset XYZ, Roll/Pitch/Yaw rotation, final local XYZ if known, and clickable controls.

Validate the 2D window behavior:

- Select `Show Runtime Tuner`; the window appears in a sane 2D position.
- Move the window to a different visible 2D position.
- Select `Show Runtime Tuner` again; the window hides.
- Select `Show Runtime Tuner`; the same window reappears at the moved position.
- Move the window again, then close it using its X-Plane window decoration/red close button.
- Select `Show Runtime Tuner`; the window should reopen on the first click at the last moved position.

Validate the clickable marker controls in 2D:

- Click `Show Marker` in the XPTO Runtime Tuner window.
- Confirm the window status changes to `Marker: shown` and local XYZ values appear.
- Click `Left/Port`, `Right/Stbd`, `Up`, `Down`, `Fore`, and `Aft`; confirm the marker moves and the XYZ display updates.
- Click `Move Step` to cycle through `1.00 m`, `0.25 m`, `0.05 m`, `0.005 m`, and `0.001 m`; confirm nudges use the selected step, including the 5 mm and 1 mm fine steps.
- Click `Rot Step` to cycle through `15 deg`, `5 deg`, `1 deg`, and `0.1 deg`.
- Click `Roll +`, `Roll -`, `Pitch +`, `Pitch -`, `Yaw +`, and `Yaw -`; confirm the marker rotates and the Roll/Pitch/Yaw display updates.
- Check `Log.txt` for `XPTO:` rotation messages showing target, body offset, Roll/Pitch/Yaw offsets, and final `XPLMDrawInfo_t` values sent to X-Plane.
- Click `Hide Marker`; confirm the marker disappears and status changes to `Marker: hidden`.

Validate VR behavior:

- Enter VR.
- Select `Plugins > XPTO > Show Runtime Tuner`.
- The same runtime tuner window should appear in the headset using `xplm_WindowVR` and show `Window mode: VR`.
- Use `Show Marker`, pilot-friendly nudge buttons, rotation buttons, `Move Step`, `Rot Step`, and `Hide Marker` from the VR window.
- Exit VR and show the window again; it should return to normal 2D floating behavior without losing the stored 2D position.

The plugin should not create duplicate runtime tuner windows or duplicate marker instances. Position, rotation, and step tracking are session-local only; they are not written to disk.

## Test Marker Instance Validation

This is an XPLM instance proof only. It does not test TDS screen rendering, cockpit-device attributes, manipulator attributes, or aircraft-attached overlay behavior.

The test marker is an original project-created OBJ at:

```text
XPTO\assets\xpto_test_marker.obj
```

It is loaded by the plugin with this X-Plane-relative path:

```text
Resources/plugins/XPTO/assets/xpto_test_marker.obj
```

Menu items remain available for quick testing:

```text
Plugins > XPTO > Show Test Marker
Plugins > XPTO > Hide Test Marker
Plugins > XPTO > Nudge Test Marker +X
Plugins > XPTO > Nudge Test Marker -X
Plugins > XPTO > Nudge Test Marker +Y
Plugins > XPTO > Nudge Test Marker -Y
Plugins > XPTO > Nudge Test Marker +Z
Plugins > XPTO > Nudge Test Marker -Z
```

Direction mapping for the tuner translation buttons:

- `Left/Port` = positive local X, formerly `X+`.
- `Right/Stbd` = negative local X, formerly `X-`.
- `Up` = positive local Y, formerly `Y+`.
- `Down` = negative local Y, formerly `Y-`.
- `Fore` = positive local Z, formerly `Z+`.
- `Aft` = negative local Z, formerly `Z-`.

Rotation mapping for the tuner buttons:

- `Roll +` / `Roll -` change the proof target roll offset, intended as rotation around the aircraft longitudinal axis.
- `Pitch +` / `Pitch -` change the proof target pitch offset, intended as rotation around the aircraft lateral axis.
- `Yaw +` / `Yaw -` change the proof target yaw offset, intended as rotation around the aircraft vertical axis.
- Rotation offsets are stored as body-relative Euler offsets in degrees for this proof.
- The current implementation sends `pitch = pitch_offset`, `heading = aircraft_true_heading + yaw_offset`, and `roll = roll_offset` through `XPLMDrawInfo_t`.
- Final sign conventions may be refined during actual overlay placement.

Coordinate/orientation frame for this proof:

- The instance uses X-Plane local/world coordinates through `XPLMInstanceSetPosition` and `XPLMDrawInfo_t`.
- Initial placement is seeded from `sim/flightmodel/position/local_x`, `local_y`, `local_z`, and `true_psi`.
- The marker starts at aircraft local position plus 8 meters forward along true heading and 3 meters up.
- Nudges directly change proof body offset X/Y/Z, currently applied directly to X-Plane local/world X/Y/Z. This is not aircraft-attached ACF coordinates and not aircraft-relative cockpit placement yet.
- Rotation buttons immediately update the marker instance with `XPLMInstanceSetPosition`.

Known unknowns:

- This does not prove that plugin-created instances can support `ATTR_cockpit_device` or `ATTR_manip_device`.
- Aircraft-relative cockpit placement will need conversion from aircraft pose/orientation to X-Plane local coordinates, likely every frame or whenever the aircraft moves.
- This first marker may still appear differently in 2D and VR depending on view, culling, and where the local offset lands relative to the camera.

Limitations:

- Buttons are simple custom-drawn rectangles using XPLM window mouse callbacks, not a full widget toolkit.
- The cursor remains the default pointer over buttons.
- Move step, rotation step, marker position, and marker rotation are session-local only.