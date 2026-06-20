# XPTO Runtime Plugin Skeleton

This folder contains the minimal native X-Plane plugin skeleton for the XPTO runtime overlay tuner.

The skeleton only:

- registers the XPTO plugin lifecycle callbacks
- adds an `XPTO` submenu under X-Plane's Plugins menu
- adds `Show Runtime Tuner`
- adds test marker instance proof menu actions
- toggles a basic modern XPLM window titled `XPTO Runtime Tuner`
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

The menu item toggles the `XPTO Runtime Tuner` window. In 2D, the window uses normal floating positioning and should display:

```text
XPTO runtime skeleton
No overlay movement implemented
Window mode: 2D floating
```

Validate the 2D toggle and session-local position behavior:

- Select `Show Runtime Tuner`; the window appears in a sane 2D position.
- Move the window to a different visible 2D position.
- Select `Show Runtime Tuner` again; the window hides.
- Select `Show Runtime Tuner`; the same window reappears at the moved position.
- Move the window again, then close it using its X-Plane window decoration/red close button.
- Select `Show Runtime Tuner`; the window should reopen on the first click at the last moved position.

Validate VR behavior:

- Enter VR.
- Select `Plugins > XPTO > Show Runtime Tuner`.
- The same runtime tuner window should appear in the headset using `xplm_WindowVR` and show `Window mode: VR`.
- Exit VR and show the window again; it should return to normal 2D floating behavior without losing the stored 2D position.

The plugin should not create duplicate runtime tuner windows. Position tracking is session-local only; it is not written to disk.

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

Use these menu items:

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

Validation steps in 2D and VR:

- Select `Show Test Marker`.
- Check `Log.txt` for `XPTO:` messages showing the object path, object load result, aircraft local position, marker initial local position, and instance creation.
- Look for a large bright magenta three-plane cross near the aircraft, initially about 8 meters forward along aircraft true heading and 3 meters above the aircraft local position.
- Use the nudge menu items and confirm the marker moves live by 1 meter per nudge.
- Select `Hide Test Marker` and confirm the marker disappears.
- Disable or unload the plugin and confirm `Log.txt` shows instance/object cleanup messages.

Coordinate frame for this proof:

- The instance uses X-Plane local/world coordinates through `XPLMInstanceSetPosition` and `XPLMDrawInfo_t`.
- Initial placement is seeded from `sim/flightmodel/position/local_x`, `local_y`, `local_z`, and `true_psi`.
- The marker starts at aircraft local position plus 8 meters forward along true heading and 3 meters up.
- Nudges directly change local X/Y/Z. This is not aircraft-attached ACF coordinates and not aircraft-relative cockpit placement yet.

Known unknowns:

- This does not prove that plugin-created instances can support `ATTR_cockpit_device` or `ATTR_manip_device`.
- Aircraft-relative cockpit placement will need conversion from aircraft pose/orientation to X-Plane local coordinates, likely every frame or whenever the aircraft moves.
- This first marker may still appear differently in 2D and VR depending on view, culling, and where the local offset lands relative to the camera.