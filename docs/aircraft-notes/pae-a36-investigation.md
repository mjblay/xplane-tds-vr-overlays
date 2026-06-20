# PAE A36 Investigation Notes

## Aircraft inspected

Local path:

- `D:\SteamLibrary\steamapps\common\X-Plane 12\Aircraft\PAE-A36`

Do not commit copied payware aircraft files from this folder.

## Top-level aircraft files observed

The aircraft has two `.acf` variants:

- `PAE_REP_A36_Analog.acf`
- `PAE_REP_A36_Glass.acf`

The aircraft has two matching VRCONFIG files:

- `PAE_REP_A36_Analog_vrconfig.txt`
- `PAE_REP_A36_Glass_vrconfig.txt`

The aircraft also includes:

- `TDSGTN.ini`
- `RealityXP.GTN.ini`
- `RealityXP.GTN.defaults.ini`
- `g5.cfg`
- `g500.cfg`

## Existing TDS objects observed

The aircraft already includes TDS GTNXi object folders:

- `objects/Instruments/TDS_GTN750NXi/`
- `objects/Instruments/TDS_GTN650NXi/`

Relevant installed object names observed in the `.acf` object list include:

- `Instruments/TDS_GTN750NXi/GTN750NXi_Analog_Panel.obj`
- `Instruments/TDS_GTN650NXi/GTN650NXi_Analog_Panel.obj`

The recursive inventory also showed glass-panel versions:

- `objects/Instruments/TDS_GTN750NXi/GTN750NXi_Glass_Panel.obj`
- `objects/Instruments/TDS_GTN650NXi/GTN650NXi_Glass_Panel.obj`

## VRCONFIG manipulator signatures observed

Both analog and glass VRCONFIG files include only a small set of manipulator blocks relevant to compatibility checks:

1. Pilot yoke:

   - `BEGIN_MANIP drag_xy sim/cockpit2/controls/yoke_roll_ratio sim/cockpit2/controls/yoke_pitch_ratio`

2. Copilot yoke:

   - `BEGIN_MANIP drag_xy sim/cockpit2/controls/yoke_roll_ratio_copilot sim/cockpit2/controls/yoke_pitch_ratio_copilot`

3. Magneto/start switch:

   - `BEGIN_MANIP command_knob sim/ignition/ignition_up_1 sim/ignition/ignition_down_1`

This means the PAE A36 may need a lighter compatibility-stub profile than the Thranda F33A if X-Plane validates the A36 VRCONFIG against added clickable overlay objects.

## ACF object block observations

The `.acf` object list uses X-Plane object-array entries like:

- `P _obja/<index>/_obj_flags <flags>`
- `P _obja/<index>/_v10_att_file_stl <object path>`
- `P _obja/<index>/_v10_att_x_acf_prt_ref <x>`
- `P _obja/<index>/_v10_att_y_acf_prt_ref <y>`
- `P _obja/<index>/_v10_att_z_acf_prt_ref <z>`
- `P _obja/<index>/_v10_att_psi_ref <heading>`
- `P _obja/<index>/_v10_att_the_ref <pitch>`
- `P _obja/<index>/_v10_att_phi_ref <roll>`

Observed object count:

- `P _obja/count 65`

The future patcher should append overlay objects by adding new `_obja` entries after the current last index and incrementing `_obja/count`.

## Initial implementation recommendation

For the first PAE A36 profile branch:

- Add a documentation-only investigation note.
- Add a preliminary profile only if it uses placeholder placement values and clearly marks the profile as experimental.
- Do not commit copied `.acf`, `.obj`, `.ini`, `.cfg`, `.txt`, `.png`, or payware inventory dumps from the aircraft folder.
- Do not patch the aircraft yet.
- First patcher support should be dry-run only.

## Open questions

- Which A36 variant should be targeted first: Analog or Glass?
- Which TDS units are active in `TDSGTN.ini`?
- Does the A36 show the same VRCONFIG warning behavior when clickable overlay objects are added?
- Are base no-stub overlays sufficient, or does the A36 need a dedicated compatibility-stub profile?
- What are the best initial overlay placements for the existing A36 TDS bezels?
## TDSGTN.ini inspection

`TDSGTN.ini` reports:

- `Installed=GTN750_1, GTN650_2`

Relevant sections observed:

- `[GTN750.1.PANEL3D]`
- `[GTN650.2.PANEL3D]`
- `[GTN750.1]`
- `[GTN650.2]`
- `[GTN750.2]`
- `[GTN650.1]`

Initial active overlay assumption:

- GTN750: Unit 1
- GTN650: Unit 2

## Existing PAE TDS object search

A quick search of the existing PAE A36 TDS GTNXi object files found TDS texture references, but did not show `ATTR_cockpit_device` or `ATTR_manip_device`.

This supports continuing with separate screen-only overlay objects for VR touchscreen behavior.

## Profile decision

Add an experimental `pae-a36` profile using the base overlay objects:

- `objects/base/TDS_GTN750_ScreenOnly_U1.obj`
- `objects/base/TDS_GTN650_ScreenOnly_U2.obj`

The profile should mark A36 compatibility stubs as unproven. If X-Plane reports VRCONFIG warnings after adding clickable overlays, create a dedicated PAE A36 compatibility object profile with hidden stubs for:

- pilot yoke: `drag_xy sim/cockpit2/controls/yoke_roll_ratio sim/cockpit2/controls/yoke_pitch_ratio`
- copilot yoke: `drag_xy sim/cockpit2/controls/yoke_roll_ratio_copilot sim/cockpit2/controls/yoke_pitch_ratio_copilot`
- magneto/start: `command_knob sim/ignition/ignition_up_1 sim/ignition/ignition_down_1`
