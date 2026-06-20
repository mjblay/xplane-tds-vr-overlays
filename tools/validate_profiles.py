from __future__ import annotations

import argparse
import json
from pathlib import Path


REQUIRED_TOP_LEVEL = {
    "schema_version",
    "profile_id",
    "aircraft",
    "matching",
    "install",
    "overlays",
    "backup_policy",
}

REQUIRED_OVERLAY = {
    "overlay_id",
    "enabled_by_default",
    "tds_unit",
    "tds_device",
    "source_object",
    "installed_object",
    "placement",
}

REQUIRED_PLACEMENT = {
    "x",
    "y",
    "z",
    "heading",
    "pitch",
    "roll",
    "scale",
}


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def require_keys(path: Path, obj: dict, keys: set[str], label: str) -> list[str]:
    errors: list[str] = []
    missing = sorted(keys - set(obj.keys()))
    for key in missing:
        errors.append(f"{path}: {label} missing required key: {key}")
    return errors


def validate_profile(path: Path, repo_root: Path) -> list[str]:
    errors: list[str] = []

    try:
        profile = load_json(path)
    except json.JSONDecodeError as exc:
        return [f"{path}: invalid JSON: {exc}"]

    errors.extend(require_keys(path, profile, REQUIRED_TOP_LEVEL, "profile"))

    if profile.get("schema_version") != 1:
        errors.append(f"{path}: schema_version must be 1")

    install = profile.get("install", {})
    object_flags = install.get("object_flags", {})
    expected_flags = {
        "clickable": True,
        "internal_cockpit": False,
        "external_cockpit": False,
    }

    for key, expected in expected_flags.items():
        actual = object_flags.get(key)
        if actual is not expected:
            errors.append(f"{path}: object_flags.{key} expected {expected}, found {actual}")

    overlays = profile.get("overlays", [])
    if not isinstance(overlays, list) or not overlays:
        errors.append(f"{path}: overlays must be a non-empty list")
        return errors

    overlay_ids: set[str] = set()

    for overlay in overlays:
        if not isinstance(overlay, dict):
            errors.append(f"{path}: overlay entry must be an object")
            continue

        errors.extend(require_keys(path, overlay, REQUIRED_OVERLAY, "overlay"))

        overlay_id = overlay.get("overlay_id")
        if overlay_id in overlay_ids:
            errors.append(f"{path}: duplicate overlay_id: {overlay_id}")
        overlay_ids.add(overlay_id)

        placement = overlay.get("placement", {})
        if isinstance(placement, dict):
            errors.extend(require_keys(path, placement, REQUIRED_PLACEMENT, f"overlay {overlay_id} placement"))
        else:
            errors.append(f"{path}: overlay {overlay_id} placement must be an object")

        source_object = overlay.get("source_object")
        if source_object:
            source_path = repo_root / source_object
            if not source_path.exists():
                errors.append(f"{path}: overlay {overlay_id} source_object not found: {source_object}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate aircraft overlay profiles.")
    parser.add_argument(
        "profiles_root",
        nargs="?",
        default="profiles/aircraft",
        help="Aircraft profile folder. Default: profiles/aircraft",
    )
    args = parser.parse_args()

    repo_root = Path.cwd()
    profiles_root = Path(args.profiles_root)

    profile_paths = sorted(profiles_root.glob("*.json"))

    if not profile_paths:
        print("No aircraft profiles found.")
        return 0

    errors: list[str] = []
    for path in profile_paths:
        errors.extend(validate_profile(path, repo_root))

    if errors:
        print("Profile validation failed:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"Profile validation passed for {len(profile_paths)} profile(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
