from __future__ import annotations

import argparse
import json
import shutil
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any


PROFILE_ROOT = Path("profiles/aircraft")


@dataclass
class AcfObject:
    index: int
    file_stl: str | None = None
    obj_flags: int | None = None
    x: float | None = None
    y: float | None = None
    z: float | None = None
    heading: float | None = None
    pitch: float | None = None
    roll: float | None = None
    xyz_field: str | None = None
    heading_field: str | None = None
    pitch_field: str | None = None
    roll_field: str | None = None


@dataclass
class OverlayMatch:
    acf_path: Path
    obj: AcfObject
    target: str


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_profiles(profile_root: Path = PROFILE_ROOT) -> dict[str, dict[str, Any]]:
    profiles: dict[str, dict[str, Any]] = {}

    for path in sorted(profile_root.glob("*.json")):
        profile = load_json(path)
        profile_id = profile.get("profile_id")
        if not profile_id:
            raise ValueError(f"{path}: missing profile_id")
        profiles[profile_id] = profile

    return profiles


def list_profiles(_: argparse.Namespace) -> int:
    profiles = load_profiles()

    if not profiles:
        print("No profiles found.")
        return 0

    for profile_id, profile in profiles.items():
        aircraft = profile.get("aircraft", {})
        name = aircraft.get("name", "(unknown aircraft)")
        vendor = aircraft.get("vendor", "(unknown vendor)")
        overlays = profile.get("overlays", [])

        print(f"{profile_id}")
        print(f"  Aircraft: {name}")
        print(f"  Vendor:   {vendor}")
        print(f"  Overlays: {len(overlays)}")
        for overlay in overlays:
            enabled = "enabled" if overlay.get("enabled_by_default") else "disabled"
            print(f"    - {overlay.get('overlay_id')} ({overlay.get('tds_unit')}, {enabled})")
        print()

    return 0

OVERLAY_PATH_MARKERS = [
    "TDS_Test/",
    "TDS_Overlay/",
    "ScreenOnly",
    "TDS_GTN750_ScreenOnly",
    "TDS_GTN650_ScreenOnly",
]


def find_existing_overlay_objects(objects: dict[int, AcfObject]) -> list[AcfObject]:
    found: list[AcfObject] = []
    for obj in objects.values():
        if not obj.file_stl:
            continue
        normalized = obj.file_stl.replace("\\", "/")
        if any(marker in normalized for marker in OVERLAY_PATH_MARKERS):
            found.append(obj)
    return sorted(found, key=lambda obj: obj.index)

def find_acf_files(aircraft_path: Path) -> list[Path]:
    return sorted(aircraft_path.glob("*.acf"))


def find_vrconfig_files(aircraft_path: Path) -> list[Path]:
    return sorted(aircraft_path.glob("*vrconfig*.txt"))


def find_tds_config_files(aircraft_path: Path) -> list[Path]:
    names = [
        "TDSGTN.ini",
        "RealityXP.GTN.ini",
        "RealityXP.GTN.defaults.ini",
    ]
    return [aircraft_path / name for name in names if (aircraft_path / name).exists()]

def find_xplane_root(path: Path) -> Path | None:
    """Best-effort detection of the X-Plane root from an aircraft path."""
    for candidate in [path, *path.parents]:
        if (candidate / "X-Plane.exe").exists():
            return candidate
        if (candidate / "Resources").exists() and (candidate / "Aircraft").exists():
            return candidate
    return None

def validate_external_backup_root(aircraft_path: Path, backup_root: Path) -> str | None:
    """Return an error message if backup_root is unsafe, otherwise None."""
    aircraft_path_resolved = aircraft_path.resolve()
    backup_root_resolved = backup_root.resolve()

    try:
        backup_root_resolved.relative_to(aircraft_path_resolved)
        return f"Backup root must not be inside the aircraft folder: {backup_root_resolved}"
    except ValueError:
        pass

    xplane_root = find_xplane_root(aircraft_path_resolved)
    if xplane_root is not None:
        try:
            backup_root_resolved.relative_to(xplane_root)
            return f"Backup root must not be inside the X-Plane folder: {backup_root_resolved}"
        except ValueError:
            pass

    return None


def parse_acf_objects(acf_path: Path) -> tuple[dict[int, AcfObject], int | None]:
    objects: dict[int, AcfObject] = {}
    object_count: int | None = None

    for line in acf_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()

        if line.startswith("P _obja/count "):
            try:
                object_count = int(line.split()[-1])
            except ValueError:
                object_count = None
            continue

        if not line.startswith("P _obja/"):
            continue

        parts = line.split(maxsplit=2)
        if len(parts) < 3:
            continue

        key = parts[1]
        value = parts[2]

        key_parts = key.split("/")
        if len(key_parts) < 3:
            continue

        try:
            index = int(key_parts[1])
        except ValueError:
            continue

        field = key_parts[2]
        obj = objects.setdefault(index, AcfObject(index=index))

        if field == "_v10_att_file_stl":
            obj.file_stl = value
        elif field == "_obj_flags":
            try:
                obj.obj_flags = int(value)
            except ValueError:
                pass
        elif field == "_obj_xyz":
            values = value.split()
            if len(values) >= 3:
                obj.x = parse_float(values[0])
                obj.y = parse_float(values[1])
                obj.z = parse_float(values[2])
                obj.xyz_field = "_obj_xyz"
        elif field == "_v10_att_x_acf_prt_ref":
            obj.x = parse_float(value)
            obj.xyz_field = obj.xyz_field or "_v10_att_xyz_refs"
        elif field == "_v10_att_y_acf_prt_ref":
            obj.y = parse_float(value)
            obj.xyz_field = obj.xyz_field or "_v10_att_xyz_refs"
        elif field == "_v10_att_z_acf_prt_ref":
            obj.z = parse_float(value)
            obj.xyz_field = obj.xyz_field or "_v10_att_xyz_refs"
        elif field == "_obj_psi":
            obj.heading = parse_float(value)
            obj.heading_field = "_obj_psi"
        elif field == "_obj_the":
            obj.pitch = parse_float(value)
            obj.pitch_field = "_obj_the"
        elif field == "_obj_phi":
            obj.roll = parse_float(value)
            obj.roll_field = "_obj_phi"
        elif field == "_v10_att_psi_ref":
            obj.heading = parse_float(value)
            obj.heading_field = "_v10_att_psi_ref"
        elif field == "_v10_att_the_ref":
            obj.pitch = parse_float(value)
            obj.pitch_field = "_v10_att_the_ref"
        elif field == "_v10_att_phi_ref":
            obj.roll = parse_float(value)
            obj.roll_field = "_v10_att_phi_ref"

    return objects, object_count


def parse_float(value: str) -> float | None:
    try:
        return float(value)
    except ValueError:
        return None

OVERLAY_TARGETS: dict[str, tuple[str, str]] = {
    "gtn750-u1": ("GTN750 U1 Overlay", "tds_gtn750_screenonly_u1"),
    "gtn650-u2": ("GTN650 U2 Overlay", "tds_gtn650_screenonly_u2"),
}


def normalize_acf_object_path(path: str) -> str:
    return path.replace("\\", "/").lower()


def target_for_overlay_path(path: str) -> str | None:
    normalized = normalize_acf_object_path(path)
    filename = normalized.rsplit("/", 1)[-1]

    if "positioningborder" in filename:
        return None

    for target, (_, marker) in OVERLAY_TARGETS.items():
        if marker in filename and filename.endswith(".obj"):
            return target

    return None


def resolve_acf_inputs(aircraft_arg: str, acf_name: str | None = None) -> tuple[Path, list[Path]]:
    aircraft_path = Path(aircraft_arg)

    if aircraft_path.suffix.lower() == ".acf":
        if acf_name is not None:
            raise ValueError("--acf cannot be used when --aircraft points directly at an .acf file.")
        if not aircraft_path.exists():
            raise ValueError(f"ACF file not found: {aircraft_path}")
        return aircraft_path.parent, [aircraft_path]

    if not aircraft_path.exists():
        raise ValueError(f"Aircraft path not found: {aircraft_path}")

    if not aircraft_path.is_dir():
        raise ValueError(f"Aircraft path is not a folder or .acf file: {aircraft_path}")

    if acf_name is not None:
        acf_path = aircraft_path / acf_name
        if not acf_path.exists():
            raise ValueError(f"ACF file not found: {acf_path}")
        return aircraft_path, [acf_path]

    return aircraft_path, find_acf_files(aircraft_path)


def find_overlay_matches(acf_paths: list[Path], target: str | None = None) -> list[OverlayMatch]:
    matches: list[OverlayMatch] = []

    for acf_path in acf_paths:
        objects, _ = parse_acf_objects(acf_path)
        for obj in objects.values():
            if obj.file_stl is None:
                continue

            matched_target = target_for_overlay_path(obj.file_stl)
            if matched_target is None:
                continue

            if target is not None and matched_target != target:
                continue

            matches.append(OverlayMatch(acf_path=acf_path, obj=obj, target=matched_target))

    return sorted(matches, key=lambda match: (match.acf_path.name, match.obj.index))


def format_optional_float(value: float | None) -> str:
    return "(missing)" if value is None else format_float(value)


def print_overlay_match(match: OverlayMatch) -> None:
    label = OVERLAY_TARGETS[match.target][0]
    obj = match.obj
    print(f"  - target: {match.target} ({label})")
    print(f"    object index: {obj.index}")
    print(f"    path: {obj.file_stl}")
    print(f"    _obj_flags: {obj.obj_flags if obj.obj_flags is not None else '(missing)'}")
    print(f"    _obj_xyz: {format_optional_float(obj.x)}, {format_optional_float(obj.y)}, {format_optional_float(obj.z)} [{obj.xyz_field or 'missing'}]")
    print(f"    _obj_psi/_obj_the/_obj_phi: {format_optional_float(obj.heading)}, {format_optional_float(obj.pitch)}, {format_optional_float(obj.roll)} [{obj.heading_field or 'missing'}, {obj.pitch_field or 'missing'}, {obj.roll_field or 'missing'}]")


def inspect_overlays(args: argparse.Namespace) -> int:
    try:
        aircraft_path, acf_paths = resolve_acf_inputs(args.aircraft, getattr(args, "acf", None))
    except ValueError as exc:
        print(exc)
        return 1

    print(f"Aircraft: {aircraft_path}")
    print()

    if not acf_paths:
        print("No ACF files found.")
        return 0

    for acf_path in acf_paths:
        objects, object_count = parse_acf_objects(acf_path)
        matches = find_overlay_matches([acf_path])
        print(f"{acf_path.name}:")
        print(f"  Parsed object entries: {len(objects)}")
        print(f"  Declared _obja/count: {object_count}")
        if not matches:
            print("  XPTO overlay objects: none found")
        else:
            print("  XPTO overlay objects:")
            for match in matches:
                print_overlay_match(match)
        print()

    return 0


def unique_overlay_match(acf_paths: list[Path], target: str) -> OverlayMatch:
    matches = find_overlay_matches(acf_paths, target)

    if not matches:
        raise ValueError(f"No ACF object entry found for target {target}.")

    if len(matches) > 1:
        locations = ", ".join(f"{match.acf_path.name}:index {match.obj.index}" for match in matches)
        raise ValueError(f"Target {target} is ambiguous; matched {len(matches)} entries: {locations}. Use --acf or a direct .acf path.")

    return matches[0]


def replace_acf_line(lines: list[str], prefix: str, replacement: str) -> bool:
    for i, line in enumerate(lines):
        if line.startswith(prefix):
            lines[i] = replacement
            return True
    return False


def patch_overlay_placement_text(
    acf_text: str,
    match: OverlayMatch,
    x: float,
    y: float,
    z: float,
    pitch: float | None = None,
    yaw: float | None = None,
    roll: float | None = None,
) -> str:
    lines = acf_text.splitlines()
    index = match.obj.index

    xyz_prefix = f"P _obja/{index}/_obj_xyz "
    if any(line.startswith(xyz_prefix) for line in lines):
        replace_acf_line(lines, xyz_prefix, f"P _obja/{index}/_obj_xyz {format_float(x)} {format_float(y)} {format_float(z)}")
    else:
        x_prefix = f"P _obja/{index}/_v10_att_x_acf_prt_ref "
        y_prefix = f"P _obja/{index}/_v10_att_y_acf_prt_ref "
        z_prefix = f"P _obja/{index}/_v10_att_z_acf_prt_ref "
        if not all(any(line.startswith(prefix) for line in lines) for prefix in [x_prefix, y_prefix, z_prefix]):
            raise ValueError(f"Target {match.target} index {index} does not have writable XYZ placement fields.")
        replace_acf_line(lines, x_prefix, f"P _obja/{index}/_v10_att_x_acf_prt_ref {format_float(x)}")
        replace_acf_line(lines, y_prefix, f"P _obja/{index}/_v10_att_y_acf_prt_ref {format_float(y)}")
        replace_acf_line(lines, z_prefix, f"P _obja/{index}/_v10_att_z_acf_prt_ref {format_float(z)}")

    angle_updates = [
        (yaw, "_obj_psi", "_v10_att_psi_ref", "yaw"),
        (pitch, "_obj_the", "_v10_att_the_ref", "pitch"),
        (roll, "_obj_phi", "_v10_att_phi_ref", "roll"),
    ]

    for value, modern_field, legacy_field, label in angle_updates:
        if value is None:
            continue

        modern_prefix = f"P _obja/{index}/{modern_field} "
        legacy_prefix = f"P _obja/{index}/{legacy_field} "
        if any(line.startswith(modern_prefix) for line in lines):
            replace_acf_line(lines, modern_prefix, f"P _obja/{index}/{modern_field} {format_float(value)}")
        elif any(line.startswith(legacy_prefix) for line in lines):
            replace_acf_line(lines, legacy_prefix, f"P _obja/{index}/{legacy_field} {format_float(value)}")
        else:
            raise ValueError(f"Target {match.target} index {index} does not have a writable {label} field.")

    return "\n".join(lines) + "\n"


def set_overlay_placement(args: argparse.Namespace) -> int:
    if args.target not in OVERLAY_TARGETS:
        print(f"Unknown target: {args.target}")
        print("Available targets:")
        for target in OVERLAY_TARGETS:
            print(f"  - {target}")
        return 1

    try:
        aircraft_path, acf_paths = resolve_acf_inputs(args.aircraft, getattr(args, "acf", None))
        match = unique_overlay_match(acf_paths, args.target)
    except ValueError as exc:
        print(exc)
        return 1

    print(f"Selected overlay in {match.acf_path.name}:")
    print_overlay_match(match)
    print()

    try:
        acf_text = match.acf_path.read_text(encoding="utf-8", errors="replace")
        patched_text = patch_overlay_placement_text(
            acf_text,
            match,
            x=args.x,
            y=args.y,
            z=args.z,
            pitch=args.pitch,
            yaw=args.yaw,
            roll=args.roll,
        )
    except ValueError as exc:
        print(exc)
        return 1

    print("Requested placement:")
    print(f"  x/y/z: {format_float(args.x)}, {format_float(args.y)}, {format_float(args.z)}")
    if args.yaw is not None:
        print(f"  yaw/_obj_psi: {format_float(args.yaw)}")
    if args.pitch is not None:
        print(f"  pitch/_obj_the: {format_float(args.pitch)}")
    if args.roll is not None:
        print(f"  roll/_obj_phi: {format_float(args.roll)}")
    print()

    if not args.apply:
        print("DRY RUN: no files were modified. Add --apply and --backup-root to write this placement.")
        return 0

    try:
        backup_root = resolve_backup_root(args, aircraft_path)
    except ValueError as exc:
        print(exc)
        return 1

    backup_error = validate_external_backup_root(aircraft_path, backup_root)
    if backup_error is not None:
        print(backup_error)
        return 1

    backup_dir = ensure_backup_dir(aircraft_path, {"backup_policy": {"include_timestamp": True}}, backup_root)
    backed_up_to = backup_file(match.acf_path, backup_dir, aircraft_path)
    match.acf_path.write_text(patched_text, encoding="utf-8", newline="\n")

    print(f"Backed up ACF: {match.acf_path} -> {backed_up_to}")
    print(f"Updated ACF placement: {match.acf_path}")
    print("Reload the aircraft in X-Plane to see the edited aircraft-attached overlay placement.")
    return 0


def inspect_aircraft(args: argparse.Namespace) -> int:
    aircraft_path = Path(args.aircraft)

    if not aircraft_path.exists():
        print(f"Aircraft path not found: {aircraft_path}")
        return 1

    print(f"Aircraft: {aircraft_path}")
    print()

    acf_files = find_acf_files(aircraft_path)
    vrconfig_files = find_vrconfig_files(aircraft_path)
    tds_config_files = find_tds_config_files(aircraft_path)

    print("ACF files:")
    if acf_files:
        for path in acf_files:
            print(f"  - {path.name}")
    else:
        print("  (none)")
    print()

    print("VRCONFIG files:")
    if vrconfig_files:
        for path in vrconfig_files:
            print(f"  - {path.name}")
    else:
        print("  (none)")
    print()

    print("Known avionics config files:")
    if tds_config_files:
        for path in tds_config_files:
            print(f"  - {path.name}")
    else:
        print("  (none)")
    print()

    for acf_path in acf_files:
        objects, object_count = parse_acf_objects(acf_path)
        print(f"{acf_path.name}:")
        print(f"  Parsed object entries: {len(objects)}")
        print(f"  Declared _obja/count: {object_count}")

        tds_like = [
            obj for obj in objects.values()
            if obj.file_stl and any(token.lower() in obj.file_stl.lower() for token in ["tds", "gtn", "gps"])
        ]

        if tds_like:
            print("  TDS/GTN/GPS-like objects:")
            for obj in tds_like:
                print(f"    - index {obj.index}: flags={obj.obj_flags} file={obj.file_stl}")
        else:
            print("  TDS/GTN/GPS-like objects: none found")

        print()

    return 0


def acf_matches_profile(acf_path: Path, profile: dict[str, Any]) -> bool:
    import fnmatch

    patterns = profile.get("matching", {}).get("acf_filename_patterns", [])
    return any(fnmatch.fnmatch(acf_path.name, pattern) for pattern in patterns)

def timestamp() -> str:
    return datetime.now().strftime("%Y%m%d-%H%M%S")

def resolve_backup_root(args: argparse.Namespace, aircraft_path: Path) -> Path:
    if not args.backup_root:
        raise ValueError("--backup-root is required with --apply. Choose a folder outside the X-Plane installation.")

    backup_root = Path(args.backup_root)

    if not backup_root.exists():
        raise ValueError(f"Backup root does not exist: {backup_root}")

    if not backup_root.is_dir():
        raise ValueError(f"Backup root is not a directory: {backup_root}")

    backup_root = backup_root.resolve()
    aircraft_path = aircraft_path.resolve()

    if backup_root == aircraft_path or aircraft_path in backup_root.parents:
        raise ValueError(f"Backup root must not be inside the aircraft folder: {backup_root}")

    # Conservative X-Plane-folder check: if the aircraft path contains an Aircraft folder,
    # treat its parent as the X-Plane root and disallow backups anywhere under it.
    xplane_root: Path | None = None
    for parent in [aircraft_path, *aircraft_path.parents]:
        if parent.name == "Aircraft":
            xplane_root = parent.parent
            break

    if xplane_root is not None and (backup_root == xplane_root or xplane_root in backup_root.parents):
        raise ValueError(f"Backup root must be outside the X-Plane folder: {backup_root}")

    test_file = backup_root / ".xpto_write_test"
    try:
        test_file.write_text("test", encoding="utf-8")
        test_file.unlink()
    except OSError as exc:
        raise ValueError(f"Backup root is not writable: {backup_root}") from exc

    return backup_root


def ensure_backup_dir(aircraft_path: Path, profile: dict[str, Any], backup_root: Path) -> Path:
    backup_policy = profile.get("backup_policy", {})
    include_timestamp = backup_policy.get("include_timestamp", True)

    aircraft_backup_root = backup_root / aircraft_path.name

    if include_timestamp:
        backup_dir = aircraft_backup_root / timestamp()
    else:
        backup_dir = aircraft_backup_root

    backup_dir.mkdir(parents=True, exist_ok=True)
    return backup_dir


def backup_file(source: Path, backup_dir: Path, aircraft_path: Path) -> Path:
    rel = source.relative_to(aircraft_path)
    destination = backup_dir / rel
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    return destination


def copy_overlay_objects(
    aircraft_path: Path,
    profile: dict[str, Any],
    backup_dir: Path,
) -> list[str]:
    messages: list[str] = []
    destination_folder = profile.get("install", {}).get("object_destination_folder", "objects/TDS_Overlay")
    destination_dir = aircraft_path / destination_folder
    destination_dir.mkdir(parents=True, exist_ok=True)

    for overlay in profile.get("overlays", []):
        if not overlay.get("enabled_by_default"):
            continue

        source = Path(overlay["source_object"])
        destination = destination_dir / overlay["installed_object"]

        if not source.exists():
            raise FileNotFoundError(f"Overlay source object not found: {source}")

        if destination.exists():
            backed_up_to = backup_file(destination, backup_dir, aircraft_path)
            messages.append(f"Backed up existing overlay: {destination} -> {backed_up_to}")

        shutil.copy2(source, destination)
        messages.append(f"Copied overlay: {source} -> {destination}")

    return messages

def format_float(value: Any) -> str:
    try:
        return f"{float(value):.9f}"
    except (TypeError, ValueError):
        return "0.000000000"


def build_acf_object_block(
    index: int,
    object_path: str,
    placement: dict[str, Any],
    obj_flags: int,
) -> list[str]:
    return [
        f"P _obja/{index}/_obj_flags {obj_flags}",
        f"P _obja/{index}/_v10_att_body -1",
        f"P _obja/{index}/_v10_att_file_stl {object_path}",
        f"P _obja/{index}/_v10_att_gear -1",
        f"P _obja/{index}/_v10_att_phi_ref {format_float(placement.get('roll'))}",
        f"P _obja/{index}/_v10_att_psi_ref {format_float(placement.get('heading'))}",
        f"P _obja/{index}/_v10_att_the_ref {format_float(placement.get('pitch'))}",
        f"P _obja/{index}/_v10_att_wing -1",
        f"P _obja/{index}/_v10_att_x_acf_prt_ref {format_float(placement.get('x'))}",
        f"P _obja/{index}/_v10_att_y_acf_prt_ref {format_float(placement.get('y'))}",
        f"P _obja/{index}/_v10_att_z_acf_prt_ref {format_float(placement.get('z'))}",
        f"P _obja/{index}/_v10_is_internal 0",
        f"P _obja/{index}/_v10_steers_with_gear 0",
    ]


def plan_install(args: argparse.Namespace) -> int:
    aircraft_path = Path(args.aircraft)

    profiles = load_profiles()
    profile = profiles.get(args.profile)

    if profile is None:
        print(f"Unknown profile: {args.profile}")
        print("Available profiles:")
        for profile_id in profiles:
            print(f"  - {profile_id}")
        return 1

    if args.patch_acf and not args.apply:
        print("--patch-acf requires --apply.")
        return 1

    if args.apply:
        print("APPLY MODE: overlay objects may be copied, and existing files may be backed up.")
        if args.patch_acf:
            print("ACF patching is enabled and will refuse files with existing overlay-like object entries.")
        else:
            print("ACF files will NOT be patched unless --patch-acf is also provided.")
    else:
        print("DRY RUN: no files will be modified.")
    print()

    print(f"Aircraft path: {aircraft_path}")
    print(f"Profile:       {profile['profile_id']}")
    print(f"Aircraft:      {profile.get('aircraft', {}).get('name')}")
    print()

    acf_files = find_acf_files(aircraft_path)
    matched_acf_files = [path for path in acf_files if acf_matches_profile(path, profile)]

    print("Matched ACF files:")
    if matched_acf_files:
        for path in matched_acf_files:
            objects, object_count = parse_acf_objects(path)
            next_index = max(objects.keys(), default=-1) + 1
            print(f"  - {path.name}: object_count={object_count}, next_index={next_index}")
    else:
        print("  (none)")
    print()

    required_files = profile.get("matching", {}).get("required_files", [])
    print("Required files:")
    for rel in required_files:
        exists = (aircraft_path / rel).exists()
        status = "OK" if exists else "MISSING"
        print(f"  - {rel}: {status}")
    print()

    destination_folder = profile.get("install", {}).get("object_destination_folder", "objects/TDS_Overlay")
    acf_object_folder = destination_folder.replace("\\", "/")

    if acf_object_folder.startswith("objects/"):
        acf_object_folder = acf_object_folder[len("objects/"):]
    print("Planned object copy operations:")
    for overlay in profile.get("overlays", []):
        if not overlay.get("enabled_by_default"):
            continue

        source = Path(overlay["source_object"])
        destination = aircraft_path / destination_folder / overlay["installed_object"]
        source_status = "OK" if source.exists() else "MISSING"

        print(f"  - {overlay['overlay_id']}")
        print(f"      source:      {source} [{source_status}]")
        print(f"      destination: {destination}")
    print()

    print("Planned ACF additions:")
    install = profile.get("install", {})
    flags = install.get("object_flags", {})
    acf_object_flags_value = install.get("acf_object_flags_value", 9)

    print(f"  semantic flags: clickable={flags.get('clickable')} internal={flags.get('internal_cockpit')} external={flags.get('external_cockpit')}")
    print(f"  ACF _obj_flags value: {acf_object_flags_value}")
    print()

    enabled_overlays = [overlay for overlay in profile.get("overlays", []) if overlay.get("enabled_by_default")]

    for acf_path in matched_acf_files:
        objects, object_count = parse_acf_objects(acf_path)
        next_index = max(objects.keys(), default=-1) + 1
        new_count = max(object_count or 0, next_index + len(enabled_overlays))

        print(f"  {acf_path.name}:")
        print(f"    current _obja/count: {object_count}")
        print(f"    planned _obja/count: {new_count}")

        existing_overlays = find_existing_overlay_objects(objects)
        if existing_overlays:
            print("    existing overlay-like objects detected:")
            for existing in existing_overlays:
                print(f"      - index {existing.index}: flags={existing.obj_flags} file={existing.file_stl}")
            print("    apply patching will refuse this ACF unless replacement support is added later.")
            print()

        for offset, overlay in enumerate(enabled_overlays):
            placement = overlay.get("placement", {})
            object_path = f"{acf_object_folder}/{overlay['installed_object']}".replace("\\", "/")
            index = next_index + offset

            print(f"    append object index {index}: {overlay['overlay_id']}")
            for line in build_acf_object_block(index, object_path, placement, acf_object_flags_value):
                print(f"      {line}")

        print(f"      P _obja/count {new_count}")
        print()
    print()

    backup_policy = profile.get("backup_policy", {})
    print("Backup policy:")
    print(f"  backup_before_patch: {backup_policy.get('backup_before_patch')}")
    print(f"  backup_folder:       {backup_policy.get('backup_folder')}")
    print(f"  include_timestamp:   {backup_policy.get('include_timestamp')}")
    print()

    if args.apply:
        print("Applying safe file operations:")
        
        try:
            backup_root = resolve_backup_root(args, aircraft_path)
        except ValueError as exc:
            print(exc)
            return 1

        backup_error = validate_external_backup_root(aircraft_path, backup_root)
        if backup_error is not None:
            print(backup_error)
            return 1

        backup_dir = ensure_backup_dir(aircraft_path, profile, backup_root)

        print(f"  Backup folder: {backup_dir}")

        for acf_path in matched_acf_files:
            backed_up_to = backup_file(acf_path, backup_dir, aircraft_path)
            print(f"  Backed up ACF: {acf_path} -> {backed_up_to}")

        for vrconfig_path in find_vrconfig_files(aircraft_path):
            backed_up_to = backup_file(vrconfig_path, backup_dir, aircraft_path)
            print(f"  Backed up VRCONFIG: {vrconfig_path} -> {backed_up_to}")

        for message in copy_overlay_objects(aircraft_path, profile, backup_dir):
            print(f"  {message}")

        if args.patch_acf:
            print()
            print("Patching ACF files:")

            for acf_path in matched_acf_files:
                objects, object_count = parse_acf_objects(acf_path)
                existing_overlays = find_existing_overlay_objects(objects)

                if existing_overlays:
                    print(f"  Refusing to patch {acf_path.name}: existing overlay-like objects detected.")
                    for existing in existing_overlays:
                        print(f"    - index {existing.index}: flags={existing.obj_flags} file={existing.file_stl}")
                    print()
                    print("ACF patching was not performed.")
                    print("Backups and overlay object copy operations may already have completed.")
                    return 1

                acf_text = acf_path.read_text(encoding="utf-8", errors="replace")
                object_blocks, new_count = build_enabled_overlay_blocks(profile, objects, object_count)
                patched_text = patch_acf_text(acf_text, objects, object_count, object_blocks)
                acf_path.write_text(patched_text, encoding="utf-8", newline="\n")
                print(f"  Patched {acf_path.name}: _obja/count {object_count} -> {new_count}")

            print()
            print("Apply complete. ACF files were patched. Reload the aircraft in X-Plane.")
        else:
            print()
            print("Apply complete. ACF files were backed up but not modified.")
            print("Use --apply --patch-acf to explicitly patch matched ACF files.")

    return 0

def list_backups(args: argparse.Namespace) -> int:
    aircraft_path = Path(args.aircraft)

    if not aircraft_path.exists():
        print(f"Aircraft path not found: {aircraft_path}")
        return 1

    if not aircraft_path.is_dir():
        print(f"Aircraft path is not a folder: {aircraft_path}")
        return 1

    backup_root = Path(args.backup_root)

    if not backup_root.exists():
        print(f"Backup root not found: {backup_root}")
        return 1

    if not backup_root.is_dir():
        print(f"Backup root is not a folder: {backup_root}")
        return 1

    backup_error = validate_external_backup_root(aircraft_path, backup_root)
    if backup_error is not None:
        print(backup_error)
        return 1

    aircraft_backup_root = backup_root.resolve() / aircraft_path.name

    if not aircraft_backup_root.exists():
        print(f"No backup folder found for aircraft: {aircraft_backup_root}")
        return 0

    backups = sorted(
        [path for path in aircraft_backup_root.iterdir() if path.is_dir()],
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )

    if not backups:
        print(f"No backups found in: {aircraft_backup_root}")
        return 0

    print(f"Backups for: {aircraft_path.name}")
    print(f"Backup root: {aircraft_backup_root}")
    print()

    for backup in backups:
        files = [path for path in backup.rglob("*") if path.is_file()]
        print(f"{backup.name}")
        print(f"  Files: {len(files)}")
        for path in files:
            rel = path.relative_to(backup)
            print(f"    - {rel}")
        print()

    return 0

def patch_acf_text(
    acf_text: str,
    objects: dict[int, AcfObject],
    object_count: int | None,
    object_blocks: list[list[str]],
) -> str:
    lines = acf_text.splitlines()
    next_index_count = max(objects.keys(), default=-1) + len(object_blocks) + 1
    new_count = max(object_count or 0, next_index_count)

    count_line_index: int | None = None
    for i, line in enumerate(lines):
        if line.startswith("P _obja/count "):
            count_line_index = i
            break

    if count_line_index is None:
        raise ValueError("ACF file does not contain P _obja/count line")

    new_lines: list[str] = []
    for block in object_blocks:
        new_lines.extend(block)

    # Insert new object blocks immediately before the count line, then update count.
    lines[count_line_index:count_line_index] = new_lines
    lines[count_line_index + len(new_lines)] = f"P _obja/count {new_count}"

    return "\n".join(lines) + "\n"


def build_enabled_overlay_blocks(
    profile: dict[str, Any],
    objects: dict[int, AcfObject],
    object_count: int | None,
) -> tuple[list[list[str]], int]:
    destination_folder = profile.get("install", {}).get("object_destination_folder", "objects/TDS_Overlay")

    acf_object_folder = destination_folder.replace("\\", "/")

    if acf_object_folder.startswith("objects/"):
        acf_object_folder = acf_object_folder[len("objects/"):]

    install = profile.get("install", {})
    acf_object_flags_value = install.get("acf_object_flags_value", 9)
    enabled_overlays = [overlay for overlay in profile.get("overlays", []) if overlay.get("enabled_by_default")]

    next_index = max(objects.keys(), default=-1) + 1
    blocks: list[list[str]] = []

    for offset, overlay in enumerate(enabled_overlays):
        placement = overlay.get("placement", {})
        object_path = f"{acf_object_folder}/{overlay['installed_object']}".replace("\\", "/")
        index = next_index + offset
        blocks.append(build_acf_object_block(index, object_path, placement, acf_object_flags_value))

    new_count = max(object_count or 0, next_index + len(enabled_overlays))
    return blocks, new_count

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="xpto",
        description="X-Plane TDS GTNXi VR overlay installer/tuner helper.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    list_profiles_parser = subparsers.add_parser("list-profiles", help="List known aircraft profiles.")
    list_profiles_parser.set_defaults(func=list_profiles)

    inspect_parser = subparsers.add_parser("inspect-aircraft", help="Inspect an aircraft folder.")
    inspect_parser.add_argument("aircraft", help="Path to aircraft folder.")
    inspect_parser.set_defaults(func=inspect_aircraft)


    inspect_overlays_parser = subparsers.add_parser("inspect-overlays", help="Inspect XPTO-installed overlay object placement in ACF files.")
    inspect_overlays_parser.add_argument("--aircraft", required=True, help="Path to aircraft folder or a prepared .acf file.")
    inspect_overlays_parser.add_argument("--acf", help="Optional ACF filename inside the aircraft folder.")
    inspect_overlays_parser.set_defaults(func=inspect_overlays)

    set_overlay_parser = subparsers.add_parser("set-overlay-placement", help="Edit placement for one XPTO-installed aircraft-attached overlay object.")
    set_overlay_parser.add_argument("--aircraft", required=True, help="Path to aircraft folder or a prepared .acf file.")
    set_overlay_parser.add_argument("--acf", help="Optional ACF filename inside the aircraft folder, used to disambiguate multi-ACF aircraft.")
    set_overlay_parser.add_argument("--target", required=True, choices=sorted(OVERLAY_TARGETS), help="Overlay target to edit.")
    set_overlay_parser.add_argument("--x", required=True, type=float, help="ACF object X placement value.")
    set_overlay_parser.add_argument("--y", required=True, type=float, help="ACF object Y placement value.")
    set_overlay_parser.add_argument("--z", required=True, type=float, help="ACF object Z placement value.")
    set_overlay_parser.add_argument("--pitch", type=float, help="Optional pitch/_obj_the value. Only updates existing ACF angle fields.")
    set_overlay_parser.add_argument("--yaw", type=float, help="Optional yaw/_obj_psi value. Only updates existing ACF angle fields.")
    set_overlay_parser.add_argument("--roll", type=float, help="Optional roll/_obj_phi value. Only updates existing ACF angle fields.")
    set_overlay_parser.add_argument("--apply", action="store_true", help="Write the edited ACF after creating an external backup.")
    set_overlay_parser.add_argument("--backup-root", help="Required with --apply. External backup root outside the aircraft and X-Plane folders.")
    set_overlay_parser.set_defaults(func=set_overlay_placement)

    plan_parser = subparsers.add_parser("plan-install", help="Plan an overlay install without modifying files.")
    plan_parser.add_argument("--aircraft", required=True, help="Path to aircraft folder.")
    plan_parser.add_argument("--profile", required=True, help="Profile ID, such as thranda-f33a or pae-a36.")
    plan_parser.add_argument(
        "--apply",
        action="store_true",
        help="Apply safe file operations: create backups and copy overlay objects. Does not patch ACF files yet.",
    )
    plan_parser.add_argument(
        "--patch-acf",
        action="store_true",
        help="With --apply, also patch matched ACF files. Refuses if existing overlay-like objects are detected.",
    )
    plan_parser.add_argument(
        "--backup-root",
        help="Required with --apply. Existing aircraft files are backed up under this external folder, outside the X-Plane folder.",
    )
    plan_parser.set_defaults(func=plan_install)

    backups_parser = subparsers.add_parser("list-backups", help="List external aircraft overlay backups.")
    backups_parser.add_argument("aircraft", help="Path to aircraft folder.")
    backups_parser.add_argument(
        "--backup-root",
        required=True,
        help="External backup root folder, outside the X-Plane folder.",
    )
    backups_parser.set_defaults(func=list_backups)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())

