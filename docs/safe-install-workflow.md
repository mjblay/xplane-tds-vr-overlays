# Safe Install Workflow

This workflow is intended for testing and installing XPTO TDS GTNXi VR overlay objects into supported X-Plane aircraft.

## Safety rules

- Do not run write operations directly against a production aircraft until the same profile has been tested on a disposable copy.
- Do not place backups anywhere inside the X-Plane installation folder.
- `--apply` requires an external `--backup-root`.
- ACF patching requires both `--apply` and `--patch-acf`.
- The tool refuses to patch an ACF that already contains overlay-like object entries, such as `TDS_Test`, `TDS_Overlay`, or `ScreenOnly`, unless future replacement support is added.

## Recommended disposable-copy workflow

```powershell
$A36 = "D:\SteamLibrary\steamapps\common\X-Plane 12\Aircraft\PAE-A36"
$A36Test = "D:\SteamLibrary\steamapps\common\X-Plane 12\Aircraft\PAE-A36-XPTO-TEST"
$BackupRoot = "D:\XPTO-Aircraft-Backups"

New-Item -ItemType Directory -Force $BackupRoot | Out-Null

Remove-Item $A36Test -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item $A36 $A36Test -Recurse

py tools\xpto.py plan-install --aircraft $A36Test --profile pae-a36
py tools\xpto.py plan-install --aircraft $A36Test --profile pae-a36 --apply --patch-acf --backup-root $BackupRoot
```

Then inspect:

```powershell
py tools\xpto.py inspect-aircraft $A36Test

Select-String -Path "$A36Test\PAE_REP_A36_Analog.acf","$A36Test\PAE_REP_A36_Glass.acf" `
  -Pattern "TDS_Overlay|P _obja/count" |
  Select-Object Path, LineNumber, Line |
  Format-Table -Wrap
```

Load the test aircraft in X-Plane and confirm:

- No `objects/objects/...` load-path error.
- No VRCONFIG dialog.
- Overlay screens render in the expected area.
- Touch/click behavior works as expected.
- Existing aircraft manipulators still work.

## PAE A36 notes

PAE A36 uses PAE-specific overlay OBJ files:

```text
objects/pae-a36/TDS_GTN750_ScreenOnly_U1_PAE_A36.obj
objects/pae-a36/TDS_GTN650_ScreenOnly_U2_PAE_A36.obj
```

These include hidden dummy VRCONFIG compatibility manipulators for:

```text
drag_xy sim/cockpit2/controls/yoke_roll_ratio_copilot sim/cockpit2/controls/yoke_pitch_ratio_copilot
command_knob sim/ignition/ignition_up_1 sim/ignition/ignition_down_1
```

The dummy manipulator geometry must reference a nonzero triangle range, such as `TRIS 0 3`; `TRIS 0 0` did not satisfy X-Plane's VRCONFIG validator.

## Thranda F33A notes

The current working F33A may already contain older test overlay entries such as:

```text
TDS_Test/TDS_GTN750_ScreenOnly_U1.obj
TDS_Test/TDS_GTN650_ScreenOnly_U2.obj
```

The installer intentionally refuses to patch an ACF when existing overlay-like entries are detected. Remove old entries manually in Plane Maker or add future explicit replacement support before patching.

## Command reference

List supported profiles:

```powershell
py tools\xpto.py list-profiles
```

Inspect an aircraft:

```powershell
py tools\xpto.py inspect-aircraft "D:\path\to\aircraft"
```

Dry-run a planned install:

```powershell
py tools\xpto.py plan-install --aircraft "D:\path\to\aircraft" --profile pae-a36
```

Copy overlays and create external backups, but do not patch ACF files:

```powershell
py tools\xpto.py plan-install --aircraft "D:\path\to\aircraft" --profile pae-a36 --apply --backup-root "D:\XPTO-Aircraft-Backups"
```

Copy overlays, create external backups, and patch matched ACF files:

```powershell
py tools\xpto.py plan-install --aircraft "D:\path\to\aircraft" --profile pae-a36 --apply --patch-acf --backup-root "D:\XPTO-Aircraft-Backups"
```

List backups:

```powershell
py tools\xpto.py list-backups "D:\path\to\aircraft"
```

Note: if `list-backups` still expects an aircraft-local backup folder, update it before documenting external backup listing as a supported feature.
