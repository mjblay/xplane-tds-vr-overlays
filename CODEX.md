## Runtime overlay tuner direction

The project is moving toward two layers:

1. Offline aircraft preparation

   * Scan an X-Plane 12 Aircraft folder.
   * Identify candidate aircraft and native GTN/GPS screen geometry.
   * Create a prepared copy of the aircraft rather than modifying payware originals by default.
   * Install XPTO overlay assets, VRCONFIG compatibility handling, and an initial runtime config.
   * Seed overlay placement from detected native screen dimensions where possible.

2. Runtime X-Plane plugin

   * Add an X-Plane plugin menu.
   * Provide a VR-compatible positioning window.
   * Toggle overlays active/inactive.
   * Toggle magenta positioning borders.
   * Select individual overlays.
   * Move overlays in x/y/z and rotate in pitch/yaw/roll without aircraft art reloads.
   * Support step sizes of 0.50 m, 0.05 m, 0.005 m, and 0.001 m.
   * Persist placement so aircraft reloads restore the last tuned positions.
   * Keep XPTO overlay state congruent with TDS GTNXi plugin state if observable.

Important constraints:

* Do not commit payware aircraft files, raw ACF dumps, raw logs, screenshots of proprietary assets, or copied third-party objects.
* Prefer small, testable branches.
* Preserve the current safe installer behavior.
* Do not build prototype cleanup/replacement support for old local `TDS_Test` entries unless explicitly requested.
* The positioning border is a runtime/tuning aid, not an installer mode.
* The current ACF object placement is a bootstrap/proof-of-concept path; the runtime plugin should eventually own live placement and persistence.
