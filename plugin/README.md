# XPTO Runtime Plugin Skeleton

This folder contains the minimal native X-Plane plugin skeleton for the XPTO runtime overlay tuner.

The skeleton only:

- registers the XPTO plugin lifecycle callbacks
- adds an `XPTO` submenu under X-Plane's Plugins menu
- adds `Show Runtime Tuner`
- toggles a basic modern XPLM window titled `XPTO Runtime Tuner`
- draws two status lines

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
D:\XPlaneSDK\XPSDK430
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
cmake -S plugin -B build\plugin -DXPLANE_SDK_ROOT="D:\XPlaneSDK\XPSDK430"
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

Start X-Plane and open:

```text
Plugins > XPTO > Show Runtime Tuner
```

The menu item toggles the `XPTO Runtime Tuner` window. The window should display:

```text
XPTO runtime skeleton
No overlay movement implemented
```
