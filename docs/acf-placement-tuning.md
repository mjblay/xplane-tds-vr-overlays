# ACF Placement Tuning

XPTO's current tuning path is file-backed: edit the placement of the working aircraft-attached overlay objects in a prepared aircraft `.acf`, then reload the aircraft in X-Plane.

## Why This Path

Runtime plugin-created XPLM instances proved that visible geometry can be moved live, but plugin-created instances of the real TDS screen-only overlay OBJs rendered as black/static rectangles and did not show live GTN content or usable input.

The working TDS GTN overlays are the aircraft-attached ACF object entries installed by the offline XPTO installer. Those objects keep the TDS `ATTR_cockpit_device` / `ATTR_manip_device` behavior active. The runtime plugin should not replace them with plugin-created overlay instances.

## CLI Commands

Inspect XPTO overlay objects in a prepared aircraft folder:

```powershell
py tools\xpto.py inspect-overlays --aircraft "D:\path\to\Prepared Aircraft"
```

Inspect one ACF directly:

```powershell
py tools\xpto.py inspect-overlays --aircraft "D:\path\to\Prepared Aircraft\Example.acf"
```

Set placement for the GTN750 U1 overlay:

```powershell
py tools\xpto.py set-overlay-placement `
  --aircraft "D:\path\to\Prepared Aircraft" `
  --target gtn750-u1 `
  --x 0.123 --y 0.456 --z -0.789 `
  --apply `
  --backup-root "D:\XPTO-Aircraft-Backups"
```

Set placement for the GTN650 U2 overlay:

```powershell
py tools\xpto.py set-overlay-placement `
  --aircraft "D:\path\to\Prepared Aircraft" `
  --target gtn650-u2 `
  --x 0.123 --y 0.456 --z -0.789 `
  --apply `
  --backup-root "D:\XPTO-Aircraft-Backups"
```

If an aircraft folder contains multiple `.acf` files with the same target, narrow the edit:

```powershell
py tools\xpto.py set-overlay-placement `
  --aircraft "D:\path\to\Prepared Aircraft" `
  --acf "Example.acf" `
  --target gtn750-u1 `
  --x 0.123 --y 0.456 --z -0.789 `
  --apply `
  --backup-root "D:\XPTO-Aircraft-Backups"
```

Dry runs omit `--apply` and never write files:

```powershell
py tools\xpto.py set-overlay-placement --aircraft "D:\path\to\Prepared Aircraft" --target gtn750-u1 --x 0 --y 0 --z 0
```

## Safety Rules

- Writes require `--apply`.
- Writes require `--backup-root`.
- The backup root must be outside the aircraft folder and outside the X-Plane folder.
- The tool backs up the ACF externally before writing.
- The tool refuses to write if the selected target is missing or ambiguous.
- The tool updates existing ACF placement fields; it does not invent new object entries or add proxy geometry.

## Target Detection

The first supported targets are:

- `gtn750-u1`
- `gtn650-u2`

The tool recognizes XPTO-style screen-only object filenames such as:

- `TDS_Overlay/TDS_GTN750_ScreenOnly_U1_F33A.obj`
- `TDS_Overlay/TDS_GTN650_ScreenOnly_U2_F33A.obj`
- base `TDS_GTN750_ScreenOnly_U1.obj` / `TDS_GTN650_ScreenOnly_U2.obj`
- PAE-style `TDS_GTN750_ScreenOnly_U1_PAE_A36.obj` / `TDS_GTN650_ScreenOnly_U2_PAE_A36.obj`

Positioning-border objects are intentionally ignored as first-class targets.

## Placement Fields

Inspection prints:

- object index
- object path
- `_obj_xyz`, or the equivalent `_v10_att_x/y/z_acf_prt_ref` values
- `_obj_psi/_obj_the/_obj_phi`, or the equivalent `_v10_att_psi/the/phi_ref` values
- `_obj_flags`

`set-overlay-placement` requires explicit `--x`, `--y`, and `--z`. Optional `--pitch`, `--yaw`, and `--roll` update existing angle fields only.

After applying an edit, reload the aircraft in X-Plane to see the aircraft-attached overlay placement change.
