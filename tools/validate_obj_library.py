from __future__ import annotations

import argparse
import re
from pathlib import Path


EXPECTED_BY_TOKEN = {
    "GTN750_ScreenOnly_U1": {
        "unit": "TDSGTN750XI_U1",
        "device": "TDS_GTN750xi",
    },
    "GTN750_ScreenOnly_U2": {
        "unit": "TDSGTN750XI_U2",
        "device": "TDS_GTN750xi",
    },
    "GTN650_ScreenOnly_U1": {
        "unit": "TDSGTN650XI_U1",
        "device": "TDS_GTN650xi",
    },
    "GTN650_ScreenOnly_U2": {
        "unit": "TDSGTN650XI_U2",
        "device": "TDS_GTN650xi",
    },
}

F33A_REQUIRED_PATTERNS = {
    "yoke drag_xy": [
        r"ATTR_manip_drag_xy[\s\S]*?sim/cockpit2/controls/yoke_roll_ratio\s+sim/cockpit2/controls/yoke_pitch_ratio",
        2,
    ],
    "ASI calibration drag_axis": [
        r"ATTR_manip_drag_axis[\s\S]*?thranda/cockpit/ASIcalib",
        1,
    ],
    "starter drag_axis": [
        r"ATTR_manip_drag_axis[\s\S]*?thranda/starter_key1",
        1,
    ],
    "fuel selector drag_axis": [
        r"ATTR_manip_drag_axis[\s\S]*?thranda/cockpit/actuators/FuelSwL",
        1,
    ],
}


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def expected_for_path(path: Path) -> dict[str, str] | None:
    for token, expected in EXPECTED_BY_TOKEN.items():
        if token in path.name:
            return expected
    return None


def validate_overlay_obj(path: Path) -> list[str]:
    errors: list[str] = []
    expected = expected_for_path(path)
    text = read_text(path)

    if expected is None:
        errors.append(f"{path}: filename does not identify a known overlay variant")
        return errors

    if "ATTR_cockpit_device" not in text:
        errors.append(f"{path}: missing ATTR_cockpit_device")

    if "ATTR_manip_device" not in text:
        errors.append(f"{path}: missing ATTR_manip_device")

    if expected["unit"] not in text:
        errors.append(f"{path}: missing expected TDS unit {expected['unit']}")

    if expected["device"] not in text:
        errors.append(f"{path}: missing expected TDS device {expected['device']}")

    return errors


def validate_f33a_obj(path: Path) -> list[str]:
    errors: list[str] = []
    text = read_text(path)

    for label, pattern_and_count in F33A_REQUIRED_PATTERNS.items():
        pattern = pattern_and_count[0]
        expected_count = pattern_and_count[1]
        found_count = len(re.findall(pattern, text))

        if found_count < expected_count:
            errors.append(
                f"{path}: expected at least {expected_count} F33A compatibility stub(s) for {label}, found {found_count}"
            )

    return errors


def is_f33a_path(path: Path) -> bool:
    parts = [part.lower() for part in path.parts]
    return "f33a" in parts or "f33a" in path.name.lower()


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate TDS GTNXi overlay OBJ library.")
    parser.add_argument(
        "root",
        nargs="?",
        default="objects",
        help="Object library root. Default: objects",
    )
    args = parser.parse_args()

    root = Path(args.root)
    errors: list[str] = []
    obj_paths = sorted(root.rglob("*.obj"))

    if not obj_paths:
        print("No OBJ files found.")
        return 0

    for path in obj_paths:
        errors.extend(validate_overlay_obj(path))

        if is_f33a_path(path):
            errors.extend(validate_f33a_obj(path))

    if errors:
        print("OBJ validation failed:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"OBJ validation passed for {len(obj_paths)} file(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
