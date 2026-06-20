# XPTO Runtime Tuner Design

## Goals
- One-time end-user aircraft preparation
- Runtime in-cockpit positioning without aircraft art reloads
- VR-compatible positioning window
- Persistent overlay placement
- Overlay active-state congruent with TDS plugin state

## Non-goals
- Manual installer-time positioning border mode
- Productizing prototype cleanup/replacement of old TDS_Test objects
- Editing payware source aircraft in place by default

## Installer responsibilities
- Scan X-Plane Aircraft folder
- Identify candidate aircraft/profile opportunities
- Detect existing native GPS/GTN screen geometry where possible
- Create a prepared copy of the aircraft
- Install XPTO overlay assets/runtime plugin/config
- Apply required VRCONFIG compatibility handling
- Seed initial overlay placement

## Runtime plugin responsibilities
- Add X-Plane menu item
- Show VR-compatible positioning window
- Toggle overlay active/inactive
- Toggle magenta positioning border
- Select active overlay
- Nudge x/y/z
- Rotate pitch/yaw/roll
- Step sizes: 0.50 m, 0.05 m, 0.005 m, 0.001 m
- Persist changes immediately or on aircraft unload
- Restore placement on aircraft load
- Keep XPTO overlay state aligned with TDS plugin menu state

## Proposed config model
- aircraft_id/profile_id
- overlay_id
- TDS unit/device
- enabled
- show_positioning_border
- x/y/z
- pitch/yaw/roll
- step size preference
- last updated timestamp

## Open SDK questions
- Can plugin-created objects use ATTR_cockpit_device / ATTR_manip_device reliably?
- Can object instances be moved live in aircraft coordinates?
- Can visibility be toggled per overlay without reloading aircraft?
- Can plugin observe TDS plugin menu selection/state directly?
- If TDS state is not observable, what fallback polling/dataref/menu strategy is available?