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
        elif field == "_v10_att_x_acf_prt_ref":
            obj.x = parse_float(value)
        elif field == "_v10_att_y_acf_prt_ref":
            obj.y = parse_float(value)
        elif field == "_v10_att_z_acf_prt_ref":
            obj.z = parse_float(value)
        elif field == "_v10_att_psi_ref":
            obj.heading = parse_float(value)
        elif field == "_v10_att_the_ref":
            obj.pitch = parse_float(value)
        elif field == "_v10_att_phi_ref":
            obj.roll = parse_float(value)

    return objects, object_count


def parse_float(value: str) -> float | None:
    try:
        return float(value)
    except ValueError:
        return None


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


def ensure_backup_dir(aircraft_path: Path, profile: dict[str, Any]) -> Path:
    backup_policy = profile.get("backup_policy", {})
    backup_folder = backup_policy.get("backup_folder", "TDS_Overlay_Backups")
    backup_dir = aircraft_path / backup_folder / timestamp()
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


def plan_install(args: argparse.Namespace) -> int:
    aircraft_path = Path(args.aircraft)

    if not aircraft_path.exists():
        print(f"Aircraft path not found: {aircraft_path}")
        return 1

    profiles = load_profiles()
    profile = profiles.get(args.profile)

    if profile is None:
        print(f"Unknown profile: {args.profile}")
        print("Available profiles:")
        for profile_id in profiles:
            print(f"  - {profile_id}")
        return 1

    if args.apply:
        print("APPLY MODE: overlay objects may be copied, and existing files may be backed up.")
        print("ACF files will NOT be patched in this version.")
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
    flags = profile.get("install", {}).get("object_flags", {})
    print(f"  object flags: clickable={flags.get('clickable')} internal={flags.get('internal_cockpit')} external={flags.get('external_cockpit')}")

    for acf_path in matched_acf_files:
        objects, _ = parse_acf_objects(acf_path)
        next_index = max(objects.keys(), default=-1) + 1

        for offset, overlay in enumerate(o for o in profile.get("overlays", []) if o.get("enabled_by_default")):
            placement = overlay.get("placement", {})
            object_path = f"{destination_folder}/{overlay['installed_object']}".replace("\\", "/")

            print(f"  - {acf_path.name} index {next_index + offset}")
            print(f"      object:  {object_path}")
            print(f"      x/y/z:   {placement.get('x')} / {placement.get('y')} / {placement.get('z')}")
            print(f"      h/p/r:   {placement.get('heading')} / {placement.get('pitch')} / {placement.get('roll')}")
            print(f"      scale:   {placement.get('scale')}")
    print()

    backup_policy = profile.get("backup_policy", {})
    print("Backup policy:")
    print(f"  backup_before_patch: {backup_policy.get('backup_before_patch')}")
    print(f"  backup_folder:       {backup_policy.get('backup_folder')}")
    print(f"  include_timestamp:   {backup_policy.get('include_timestamp')}")
    print()

    if args.apply:
        print("Applying safe file operations:")
        backup_dir = ensure_backup_dir(aircraft_path, profile)
        print(f"  Backup folder: {backup_dir}")

        for acf_path in matched_acf_files:
            backed_up_to = backup_file(acf_path, backup_dir, aircraft_path)
            print(f"  Backed up ACF: {acf_path} -> {backed_up_to}")

        for vrconfig_path in find_vrconfig_files(aircraft_path):
            backed_up_to = backup_file(vrconfig_path, backup_dir, aircraft_path)
            print(f"  Backed up VRCONFIG: {vrconfig_path} -> {backed_up_to}")

        for message in copy_overlay_objects(aircraft_path, profile, backup_dir):
            print(f"  {message}")

        print()
        print("Apply complete. ACF files were backed up but not modified.")
        print("Next phase will add explicit ACF patching.")

    return 0

def list_backups(args: argparse.Namespace) -> int:
    aircraft_path = Path(args.aircraft)

    if not aircraft_path.exists():
        print(f"Aircraft path not found: {aircraft_path}")
        return 1

    backup_root = aircraft_path / args.backup_folder

    if not backup_root.exists():
        print(f"No backup folder found: {backup_root}")
        return 0

    backups = sorted(
        [path for path in backup_root.iterdir() if path.is_dir()],
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )

    if not backups:
        print(f"No backups found in: {backup_root}")
        return 0

    print(f"Backups in: {backup_root}")
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

    plan_parser = subparsers.add_parser("plan-install", help="Plan an overlay install without modifying files.")
    plan_parser.add_argument("--aircraft", required=True, help="Path to aircraft folder.")
    plan_parser.add_argument("--profile", required=True, help="Profile ID, such as thranda-f33a or pae-a36.")
    plan_parser.add_argument(
        "--apply",
        action="store_true",
        help="Apply safe file operations: create backups and copy overlay objects. Does not patch ACF files yet.",
    )
    plan_parser.set_defaults(func=plan_install)

    backups_parser = subparsers.add_parser("list-backups", help="List aircraft-local overlay backups.")
    backups_parser.add_argument("aircraft", help="Path to aircraft folder.")
    backups_parser.add_argument(
        "--backup-folder",
        default="TDS_Overlay_Backups",
        help="Backup folder name inside the aircraft folder. Default: TDS_Overlay_Backups",
    )
    backups_parser.set_defaults(func=list_backups)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
