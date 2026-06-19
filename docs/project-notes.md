# Project Notes

## Goal

Build a public, GitHub-backed X-Plane 12 tool/plugin named xplane-tds-vr-overlays that can install and tune TDS GTNXi VR touchscreen overlay objects.

The first reliable version should be a file-backed installer/tuner. It should patch aircraft files, save profiles, maintain backups, and ask the user to reload the aircraft when needed.

Do not assume fully dynamic runtime object injection will correctly support ATTR_cockpit_device / ATTR_manip_device.

## Known working overlay objects

Four variants are needed:

1. GTN750Xi Unit 1
   - TDS unit: TDSGTN750XI_U1
   - device/texture: TDS_GTN750xi

2. GTN750Xi Unit 2
   - TDS unit: TDSGTN750XI_U2
   - device/texture: TDS_GTN750xi

3. GTN650Xi Unit 1
   - TDS unit: TDSGTN650XI_U1
   - device/texture: TDS_GTN650xi

4. GTN650Xi Unit 2
   - TDS unit: TDSGTN650XI_U2
   - device/texture: TDS_GTN650xi

## Required object flags

When added to the aircraft in Plane Maker:

- clickable: checked
- internal cockpit: unchecked
- external cockpit: unchecked

## Thranda F33A proof

Working pair:

- GTN750 overlay: U1
- GTN650 overlay: U2

Behavior:

- both overlays render
- both overlays accept 2D/VR touchscreen input
- original Thranda bezel/hard keys still control the corresponding TDS navigators
- original legacy GTN screen may stop updating, which is acceptable

## F33A VRCONFIG compatibility discovery

Adding clickable overlay objects caused X-Plane VRCONFIG warning dialogs.

Observed behavior suggests X-Plane validates the aircraft's rconfig.txt manipulator blocks against each added clickable overlay object.

Deleting F33A VRCONFIG manipulator blocks removed the dialogs but broke important VR behavior, so deletion is not acceptable.

Correct compatibility approach:

- preserve the original aircraft rconfig.txt
- include hidden/dummy manipulator stubs inside every added clickable overlay object

For the Thranda F33A, each clickable overlay object needs hidden stubs matching these required VRCONFIG manipulator signatures:

1. Pilot's yoke

   drag_xy sim/cockpit2/controls/yoke_roll_ratio sim/cockpit2/controls/yoke_pitch_ratio

2. Copilot's yoke

   drag_xy sim/cockpit2/controls/yoke_roll_ratio sim/cockpit2/controls/yoke_pitch_ratio

3. ASI calibration

   drag_axis thranda/cockpit/ASIcalib

4. Starter

   drag_axis thranda/starter_key1

5. Fuel selector

   drag_axis thranda/cockpit/actuators/FuelSwL

Adding these hidden compatibility stubs to the 750 overlay reduced the VRCONFIG warnings by half.

Adding the same stubs to the 650 overlay removed the remaining warnings.

Overlay clicks still worked and F33A VR behavior was restored.

## First implementation bias

Start simple:

- object files in repo
- aircraft profiles in JSON or YAML
- CLI patcher with dry-run
- backups before writes
- restore command
- profile-driven object selection
- later investigate plugin/runtime tuning
