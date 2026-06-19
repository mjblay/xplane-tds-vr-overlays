# Roadmap

## Phase 0 - Repo setup

- [x] Create repository skeleton
- [x] Add README
- [x] Add license
- [x] Add project notes
- [ ] Push to GitHub

## Phase 1 - Object library

- [ ] Add four base overlay OBJ variants
- [ ] Add F33A-stubbed variants or generated stub injection
- [ ] Add OBJ validation script
- [ ] Add tests for required ATTR_cockpit_device and ATTR_manip_device lines

## Phase 2 - Aircraft profile format

- [ ] Define JSON/YAML schema
- [ ] Add Thranda F33A profile
- [ ] Encode backup policy
- [ ] Encode required object flags

## Phase 3 - Installer/patcher CLI

- [ ] Detect aircraft folder
- [ ] Copy overlay objects
- [ ] Patch .acf
- [ ] Back up modified files
- [ ] Restore backups
- [ ] List installed overlays
- [ ] Dry-run mode

## Phase 4 - F33A proof profile

- [ ] Encode 750 U1 + 650 U2 setup
- [ ] Preserve original rconfig.txt
- [ ] Include F33A compatibility stubs in every clickable overlay object

## Phase 5 - Tuner workflow

- [ ] Edit profile placement values
- [ ] Rewrite .acf transforms
- [ ] Add magenta outline tuning object variants
- [ ] Hide outline in final mode

## Phase 6 - TDS navigator detection

- [ ] Investigate TDS config files
- [ ] Detect configured 750/650 U1/U2 units
- [ ] Suggest overlay choices

## Later

- [ ] Smart knob improvements for painful VR knobs
