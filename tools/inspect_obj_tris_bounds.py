from pathlib import Path
import sys
import math

"""Inspect OBJ8 TRIS groups and print bounding boxes.

This is a development aid for finding screen/display mesh dimensions in
third-party aircraft objects. It supports both IDX and IDX10 index records.
"""

def parse_obj(path: Path):
    vertices = []
    indices = []
    tris_groups = []

    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue

        parts = stripped.split()
        op = parts[0]

        if op == "VT":
            x, y, z = map(float, parts[1:4])
            vertices.append((x, y, z))

        elif op == "IDX":
            indices.append(int(parts[1]))

        elif op == "IDX10":
            indices.extend(int(value) for value in parts[1:])

        elif op == "TRIS":
            start = int(parts[1])
            count = int(parts[2])
            tris_groups.append((start, count))

    return vertices, indices, tris_groups

def bbox_for_group(vertices, indices, start, count):
    idxs = indices[start:start + count]
    pts = [vertices[i] for i in idxs if 0 <= i < len(vertices)]
    if not pts:
        return None

    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    zs = [p[2] for p in pts]

    dx = max(xs) - min(xs)
    dy = max(ys) - min(ys)
    dz = max(zs) - min(zs)

    dims = sorted([dx, dy, dz], reverse=True)
    aspect = dims[0] / dims[1] if dims[1] else math.inf

    return {
        "dx": dx,
        "dy": dy,
        "dz": dz,
        "major": dims[0],
        "minor": dims[1],
        "aspect": aspect,
        "min": (min(xs), min(ys), min(zs)),
        "max": (max(xs), max(ys), max(zs)),
    }

def report(path: Path):
    vertices, indices, tris_groups = parse_obj(path)

    print()
    print(path)
    print(f"vertices={len(vertices)} indices={len(indices)} tris_groups={len(tris_groups)}")
    print()

    rows = []
    for group_i, (start, count) in enumerate(tris_groups):
        box = bbox_for_group(vertices, indices, start, count)
        if box is None:
            rows.append((group_i, start, count, None))
        else:
            rows.append((group_i, start, count, box))

    for group_i, start, count, box in rows:
        if box is None:
            print(f"group={group_i:02d} TRIS {start:6d} {count:5d} NO VALID POINTS")
            continue

        print(
            f"group={group_i:02d} TRIS {start:6d} {count:5d} "
            f"dx={box['dx']:.4f} dy={box['dy']:.4f} dz={box['dz']:.4f} "
            f"major={box['major']:.4f} minor={box['minor']:.4f} aspect={box['aspect']:.3f} "
            f"min=({box['min'][0]:.4f},{box['min'][1]:.4f},{box['min'][2]:.4f}) "
            f"max=({box['max'][0]:.4f},{box['max'][1]:.4f},{box['max'][2]:.4f})"
        )

def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: py tools/inspect_obj_tris_bounds.py <obj-file> [<obj-file> ...]")
        return 1

    exit_code = 0

    for arg in sys.argv[1:]:
        path = Path(arg)
        if not path.exists():
            print()
            print(f"Missing file: {path}")
            exit_code = 1
            continue

        if not path.is_file():
            print()
            print(f"Not a file: {path}")
            exit_code = 1
            continue

        report(path)

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
