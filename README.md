# xplane-tds-vr-overlays

Experimental installer/tuner for X-Plane 12 TDS GTNXi VR touchscreen overlay objects.

This project exists because screen-only OBJ overlays using X-Plane cockpit device attributes can make TDS GTNXi screens usable in VR when installed as aircraft objects with the correct flags.

## Proven starting facts

Working overlay OBJ technique:

- ATTR_cockpit_device
- ATTR_manip_device
- screen-only OBJ overlay geometry

Working Plane Maker object flags:

- clickable: checked
- internal cockpit: unchecked
- external cockpit: unchecked

Known overlay variants:

| Overlay | TDS device name | Cockpit texture/device |
| --- | --- | --- |
| GTN750Xi Unit 1 | TDSGTN750XI_U1 | TDS_GTN750xi |
| GTN750Xi Unit 2 | TDSGTN750XI_U2 | TDS_GTN750xi |
| GTN650Xi Unit 1 | TDSGTN650XI_U1 | TDS_GTN650xi |
| GTN650Xi Unit 2 | TDSGTN650XI_U2 | TDS_GTN650xi |

Known working Thranda F33A setup:

- GTN750 overlay: Unit 1
- GTN650 overlay: Unit 2
- both overlays accept touchscreen input
- original Thranda bezel/hard keys still control the corresponding TDS navigators
- original legacy GTN screen may stop updating, which is acceptable because the overlay replaces it

## Safety goals

This project should always be safe and reversible.

- Dry-run before modifying aircraft files.
- Back up every changed file before patching.
- Never delete aircraft rconfig.txt manipulator blocks as a compatibility strategy.
- Keep per-aircraft overlay configuration outside the original aircraft files where practical.
- Prefer small, testable changes.

## Windows Python note

On Windows, use the Python launcher in examples:

````powershell
py tools\validate_obj_library.py
````

## Initial phases

1. Repo setup.
2. Object library.
3. Aircraft profile format.
4. Installer/patcher CLI.
5. Thranda F33A proof profile.
6. Tuner workflow.
7. TDS navigator detection.
8. Optional smart knob improvements.

## Status

Phase 0: repository setup.
