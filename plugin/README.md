# XPTO Runtime Placement Proxy

This plugin is a developer-only calibration tool. It is not part of the intended public release path.

Public release should remain the installer/profile/overlay-object workflow with correctly placed aircraft-attached overlays. The runtime proxy exists only to help developers measure candidate placement values.

The important project finding is fixed in the design: plugin-created XPLM instances can move visible geometry, but plugin-created instances of the real TDS screen-only overlay OBJs rendered as black/static rectangles and did not provide live GTN content or input. The working TDS overlays are aircraft-attached ACF objects installed by the offline installer.

Therefore this plugin does not create plugin-owned TDS overlay instances. It creates only a visible measuring proxy.

## Current Scope

The plugin currently:

- opens the `XPTO Runtime Tuner` window
- creates project-owned proxy OBJ instances only
- cycles targets:
  - `GTN750 U1`
  - `GTN650 U2`
- initializes proxy placement from F33A ACF candidate defaults
- moves, rotates, and resizes the selected proxy in real time
- stores developer calibration records in `Resources/plugins/XPTO/config/xpto-proxy-calibrations.json`
- exports corrected ACF candidate placement JSON to `Resources/plugins/XPTO/exports/xpto-placement-export.json`
- converts ACF/body-frame placement into X-Plane local coordinates only for rendering

It does not edit aircraft files, modify installer behavior, load TDS overlay OBJs as plugin instances, or write ACF placement directly.

## F33A Defaults

Resetting a proxy restores these ACF candidate placement values:

```text
GTN750 U1: x=1.360000014 y=0.899999976 z=-1.350000024 pitch=0 yaw=0 roll=0
GTN650 U2: x=1.360000014 y=0.300000012 z=-1.350000024 pitch=0 yaw=0 roll=0
```

## Build

```powershell
cmake -S plugin -B build\plugin -DXPLANE_SDK_ROOT="D:\xplane-dev\sdk\XPSDK\SDK"
cmake --build build\plugin --config Release
```

The Windows build stages:

```text
build\plugin\XPTO\win_x64\XPTO.xpl
build\plugin\XPTO\assets\xpto_gtn750_proxy.obj
build\plugin\XPTO\assets\xpto_gtn650_proxy.obj
```

## Developer Workflow

1. Run `py tools\xpto.py inspect-overlays --aircraft "<aircraft path>"` to inspect the current real ACF-attached overlay placement.
2. Seed or edit `Resources/plugins/XPTO/config/xpto-proxy-calibrations.json` with the current baseline if needed.
3. Open X-Plane with the prepared aircraft.
4. Open `Plugins > XPTO > Show Runtime Tuner`.
5. Click `Target` to choose `GTN750 U1` or `GTN650 U2`.
6. Click `Show Proxy`.
7. Move, rotate, and resize the proxy until it visually matches the current real overlay/native screen.
8. Click `Calibrate Here` to save the aircraft/target calibration record.
9. Move the proxy to the desired new placement and click `Export`.
10. Apply the exported corrected ACF candidate values to the real aircraft-attached ACF overlay object with the CLI.
11. Reload the aircraft in X-Plane.

The calibration file is developer-owned and safe to edit manually:

```text
X-Plane 12\Resources\plugins\XPTO\config\xpto-proxy-calibrations.json
```

Each record is keyed by aircraft folder, ACF filename, optional profile id, and target id. A record includes `acf_baseline_placement`, `proxy_source_at_visual_match`, `calibration_offset`, size, timestamp, and notes. `Calibrate Here` computes `calibration_offset = acf_baseline_placement - proxy_source_at_visual_match`.

The export file is:

```text
X-Plane 12\Resources\plugins\XPTO\exports\xpto-placement-export.json
```

The JSON `proxy_source_placement` object is the raw developer jig source. When calibrated, the JSON `placement` object is corrected with `proxy_source_placement + calibration_offset` and is intended as the ACF candidate placement. If not calibrated, the export includes `calibrated=false` and a warning; `placement` then equals the raw proxy source. `debug_final_local` is only the rendered X-Plane local/world position.

## Controls

Movement modifies only ACF candidate x/y/z and does not depend on proxy rotation:

```text
Left / Port       = +X
Right / Starboard = -X
Up                = +Y
Down              = -Y
Fore              = +Z
Aft               = -Z
```

Rotation modifies only pitch/yaw/roll. Resize modifies only proxy width/height. `Reset Proxy` restores the saved visual-match source when calibrated, or the seeded/current baseline when not calibrated. `Clear Cal` marks the selected aircraft/target record uncalibrated without editing any aircraft file.

The base proxy dimensions match the current screen-only overlay quads:

```text
GTN750: half-size x=0.0661, y=0.0750
GTN650: half-size x=0.0661, y=0.0330
```

The proxy is a dark rectangle with a bright magenta border. It is only a measuring jig and is not the final overlay.
