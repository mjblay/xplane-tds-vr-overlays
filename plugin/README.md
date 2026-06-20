# XPTO Runtime Plugin Skeleton

This folder contains the minimal native X-Plane plugin skeleton for the XPTO runtime overlay tuner.

The skeleton only:

- registers the XPTO plugin lifecycle callbacks
- adds an `XPTO` submenu under X-Plane's Plugins menu
- adds `Show Runtime Tuner`
- toggles a basic modern XPLM window titled `XPTO Runtime Tuner`
- uses a normal floating XPLM window with session-local geometry tracking
- draws three status lines

It does not load OBJ files, move overlays, edit aircraft files, or modify installer behavior.

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
```

## Manual Install

Copy the staged `XPTO` folder to:

```text
X-Plane 12\Resources\plugins\XPTO
```

Expected installed layout:

```text
X-Plane 12\Resources\plugins\XPTO\win_x64\XPTO.xpl
```

## Validation

Start X-Plane and open:

```text
Plugins > XPTO > Show Runtime Tuner
```

The menu item toggles the `XPTO Runtime Tuner` window. On first show, the window should appear in a reasonable visible 2D location rather than the lower-left corner. It should display:

```text
XPTO runtime skeleton
No overlay movement implemented
Window mode: 2D floating
```

Validate the toggle and session-local position behavior:

- Select `Show Runtime Tuner`; the window appears in a sane 2D position.
- Move the window to a different visible 2D position.
- Select `Show Runtime Tuner` again; the window hides.
- Select `Show Runtime Tuner`; the same window reappears at the moved position.
- Move the window again, then close it using its X-Plane window decoration/red close button.
- Select `Show Runtime Tuner`; the window should reopen on the first click at the last moved position.

The plugin should not create duplicate runtime tuner windows. Position tracking is session-local only; it is not written to disk.