# Runtime Proxy Export Workflow

This runtime plugin workflow is a developer-only calibration aid. It is not the public release path.

Public release should remain the installer/profile/overlay-object workflow with correctly placed aircraft-attached overlays. The proxy is only a measuring jig used to discover candidate placement values.

## Why

Plugin-created XPLM instances can move visible geometry. However, plugin-created instances of the real TDS screen-only overlay OBJs rendered as black/static rectangles and did not display live GTN content or accept useful input.

The real working TDS GTN overlays are aircraft-attached ACF object entries installed by the offline installer. Those ACF-attached objects keep the TDS `ATTR_cockpit_device` / `ATTR_manip_device` behavior active.

## Coordinate Model

The proxy source of truth is ACF candidate placement:

- x/y/z
- pitch/yaw/roll
- width/height

The plugin converts those ACF/body-frame values to X-Plane local/world coordinates only for `XPLMInstanceSetPosition` rendering. Exported `placement` values are not debug-anchor offsets and are not local-world coordinates.

Reset defaults for the F33A calibration case:

```text
GTN750 U1: x=1.360000014 y=0.899999976 z=-1.350000024 pitch=0 yaw=0 roll=0
GTN650 U2: x=1.360000014 y=0.300000012 z=-1.350000024 pitch=0 yaw=0 roll=0
```

Movement buttons modify only one ACF candidate axis at a time:

```text
Left / Port       = +X
Right / Starboard = -X
Up                = +Y
Down              = -Y
Fore              = +Z
Aft               = -Z
```

## Calibration Records

Persistent calibration is aircraft/profile/target-specific developer data. It is stored under the deployed plugin folder:

```text
Resources/plugins/XPTO/config/xpto-proxy-calibrations.json
```

The file is safe to seed or edit manually. Records are keyed by aircraft folder, ACF filename, optional profile id, and target id. Each record includes:

- `acf_baseline_placement`: the current real ACF-attached overlay placement, usually copied from `inspect-overlays`
- `proxy_source_at_visual_match`: the proxy source values when the proxy visually matches the current real overlay/native screen
- `calibration_offset`: `acf_baseline_placement - proxy_source_at_visual_match`
- `size`: proxy width/height and scale
- timestamp and notes

`Reset Proxy` uses the saved visual-match source when calibrated. If a record is only seeded but not calibrated, reset uses the seeded baseline as the developer starting point.

## Workflow

1. Use `py tools\xpto.py inspect-overlays --aircraft "<aircraft path>"` to inspect the current ACF-attached overlay values.
2. Seed or edit the calibration JSON baseline values if needed.
3. Use the runtime plugin to show a proxy for `GTN750 U1` or `GTN650 U2`.
4. Move, rotate, and resize the proxy until it visually matches the current real overlay/native screen.
5. Click `Calibrate Here`; the plugin saves the calibration offset for the current aircraft/target.
6. Move the proxy to the desired new placement.
7. Export `Resources/plugins/XPTO/exports/xpto-placement-export.json`.
8. Use the CLI ACF placement tools to apply the corrected `placement` values to the real aircraft-attached overlay object.
9. Reload the aircraft in X-Plane.

## Export Data

The export includes:

- target id/name
- timestamp
- aircraft file/path when available
- `calibrated`: whether a calibration record was active
- aircraft identity and calibration path
- `proxy_source_placement`: raw developer jig x/y/z/pitch/yaw/roll
- `calibration_offset`: the correction added to raw proxy source values
- `placement`: corrected ACF candidate x/y/z/pitch/yaw/roll, or raw source when uncalibrated
- `placement_note`: reminder that placement is corrected ACF candidate data
- `calibration_warning` when exporting without calibration
- width/height and scale
- `debug_final_local`: rendered X-Plane local/world position/orientation only for debugging

## Related CLI Path

See [ACF Placement Tuning](acf-placement-tuning.md) for the file-backed CLI path that edits prepared aircraft `.acf` files with external backups.

Public releases should consume curated known-good overlay placements through the installer/profile/object workflow. This runtime plugin remains a developer calibration aid only.
