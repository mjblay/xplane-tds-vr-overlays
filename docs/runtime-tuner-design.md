# XPTO Runtime Tuner Design

## Current finding

Plugin-created XPLM instances can move visible geometry, but plugin-created instances of the real TDS screen-only overlay OBJs rendered as black/static rectangles and did not display live GTN content or accept useful input.

The working TDS GTN overlays are the aircraft-attached ACF object entries installed by the offline XPTO installer. XPTO should not try to replace those working ACF-attached overlays with plugin-created overlay instances.

The near-term tuning path is documented in [ACF Placement Tuning](acf-placement-tuning.md): edit the prepared aircraft `.acf` overlay object placement, keep an external backup, and reload the aircraft.

## Goals

- One-time end-user aircraft preparation
- Safe inspection of XPTO-installed aircraft-attached overlay object entries
- File-backed placement edits with external backups
- Aircraft reload workflow for validating placement
- Preserve working TDS `ATTR_cockpit_device` / `ATTR_manip_device` behavior by keeping overlays aircraft-attached

## Non-goals

- Replacing working ACF-attached TDS overlays with plugin-created instances
- Building more marker, proxy, or magenta-border infrastructure as the primary target
- Editing payware source aircraft in place by default
- Writing placement changes without an external backup

## Installer responsibilities

- Scan X-Plane Aircraft folder
- Identify candidate aircraft/profile opportunities
- Detect existing native GPS/GTN screen geometry where possible
- Create a prepared copy of the aircraft
- Install XPTO overlay assets/config
- Apply required VRCONFIG compatibility handling
- Seed initial overlay placement as aircraft-attached ACF object entries

## Tooling responsibilities

- Inspect prepared aircraft `.acf` files
- Find XPTO-installed overlay object entries by object path
- Print object index, path, placement, rotation, and flags
- Apply explicit placement edits only when the target is unique
- Require `--apply` and an external `--backup-root` before writing
- Back up the ACF externally before every write
- Ask the user to reload the aircraft after edits

## Deferred runtime plugin questions

- Can a plugin observe TDS plugin menu selection/state directly?
- Can a plugin help display current tuning values without owning the overlay object?
- Is there any SDK-supported way to influence aircraft-attached object placement live without reloading the aircraft?
