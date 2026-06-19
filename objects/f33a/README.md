# Thranda F33A Compatibility Objects

The Thranda F33A keeps its original rconfig.txt.

Do not delete F33A VRCONFIG manipulator blocks.

X-Plane appears to validate the aircraft's VRCONFIG manipulator blocks against each added clickable overlay object. Therefore, every added clickable TDS overlay OBJ needs hidden/dummy manipulator stubs matching the required F33A VRCONFIG manipulator signatures.

Required F33A compatibility manipulator signatures:

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

Known working F33A overlay pair:

- GTN750: U1
- GTN650: U2
