# Aircraft Profiles

Aircraft profiles describe how this tool should install and tune TDS GTNXi VR overlays for a specific aircraft.

Profiles are intentionally separate from the installer logic so that aircraft-specific knowledge can be added without rewriting the patcher.

## Current profile schema

Schema file:

- schemas/aircraft-profile.schema.json

Profile folder:

- profiles/aircraft

## Profile responsibilities

A profile defines:

- aircraft name and vendor
- matching rules for candidate .acf files
- overlay object destination folder
- required object flags
- compatibility stub profile
- overlay source objects
- default placement values
- backup policy
- aircraft-specific notes

## Object flags

Known working flags for TDS GTNXi VR overlay objects:

- clickable: true
- internal cockpit: false
- external cockpit: false

## Thranda F33A

Profile:

- profiles/aircraft/thranda-f33a.json

Known working overlays:

- GTN750 U1
- GTN650 U2

The F33A profile preserves the original aircraft rconfig.txt.

F33A compatibility is handled by using overlay OBJ files that include hidden/dummy manipulators matching the required F33A VRCONFIG signatures.
