"""Generate OBJ8 positioning-border variants for XPTO screen overlay objects.

The generated object keeps the original overlay unchanged and adds a thin
magenta rectangular border around the first visible screen quad. This is meant
to help visually position overlays in Plane Maker / X-Plane.
"""

from __future__ import annotations

import argparse
from pathlib import Path


BORDER_SUFFIX = "_PositioningBorder"


def parse_point_counts(line: str) -> tuple[int, int, int, int]:
    parts = line.split()
    if len(parts) != 5 or parts[0] != "POINT_COUNTS":
        raise ValueError(f"Invalid POINT_COUNTS line: {line!r}")
    return tuple(int(value) for value in parts[1:5])  # type: ignore[return-value]


def format_vt(x: float, y: float, z: float, u: float, v: float) -> str:
    return f"VT {x: .4f} {y: .4f} {z: .4f}   0.0 0.0 1.0   {u:.1f} {v:.1f}"


def find_first_screen_vertices(lines: list[str]) -> list[tuple[float, float, float]]:
    vertices: list[tuple[float, float, float]] = []

    for line in lines:
        stripped = line.strip()
        if not stripped.startswith("VT "):
            continue

        parts = stripped.split()
        vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))

        if len(vertices) == 4:
            return vertices

    raise ValueError("Could not find first four VT screen vertices.")


def make_border_vertices(
    screen_vertices: list[tuple[float, float, float]],
    thickness: float,
    z_offset: float,
) -> list[str]:
    xs = [vertex[0] for vertex in screen_vertices]
    ys = [vertex[1] for vertex in screen_vertices]
    z = screen_vertices[0][2] + z_offset

    x_min = min(xs)
    x_max = max(xs)
    y_min = min(ys)
    y_max = max(ys)

    outer_x_min = x_min - thickness
    outer_x_max = x_max + thickness
    outer_y_min = y_min - thickness
    outer_y_max = y_max + thickness

    # Four rectangles: top, bottom, left, right.
    rects = [
        # top
        (outer_x_min, outer_y_max, outer_x_max, y_max),
        # bottom
        (outer_x_min, y_min, outer_x_max, outer_y_min),
        # left
        (outer_x_min, y_max, x_min, y_min),
        # right
        (x_max, y_max, outer_x_max, y_min),
    ]

    vt_lines: list[str] = []

    for left, top, right, bottom in rects:
        vt_lines.extend(
            [
                format_vt(left, top, z, 0.0, 1.0),
                format_vt(right, top, z, 1.0, 1.0),
                format_vt(right, bottom, z, 1.0, 0.0),
                format_vt(left, bottom, z, 0.0, 0.0),
            ]
        )

    return vt_lines


def make_border_indices(first_new_vertex_index: int) -> list[str]:
    idx_lines: list[str] = []

    for rect in range(4):
        base = first_new_vertex_index + rect * 4
        idx_lines.extend(
            [
                f"IDX {base}",
                f"IDX {base + 1}",
                f"IDX {base + 2}",
                f"IDX {base}",
                f"IDX {base + 2}",
                f"IDX {base + 3}",
            ]
        )

    return idx_lines


def make_output_path(source: Path) -> Path:
    stem = source.stem
    if stem.endswith(BORDER_SUFFIX):
        return source

    return source.with_name(f"{stem}{BORDER_SUFFIX}{source.suffix}")


def generate_positioning_border(source: Path, output: Path, thickness: float, z_offset: float) -> None:
    lines = source.read_text(encoding="utf-8", errors="replace").splitlines()

    point_counts_index = next(
        (index for index, line in enumerate(lines) if line.strip().startswith("POINT_COUNTS ")),
        None,
    )
    if point_counts_index is None:
        raise ValueError(f"No POINT_COUNTS line found in {source}")

    vertex_count, line_count, light_count, index_count = parse_point_counts(lines[point_counts_index])
    screen_vertices = find_first_screen_vertices(lines)

    border_vertices = make_border_vertices(screen_vertices, thickness=thickness, z_offset=z_offset)
    border_indices = make_border_indices(first_new_vertex_index=vertex_count)

    new_vertex_count = vertex_count + len(border_vertices)
    new_index_count = index_count + len(border_indices)

    lines[point_counts_index] = f"POINT_COUNTS {new_vertex_count} {line_count} {light_count} {new_index_count}"

    # Append vertices and indices before attributes/commands. This works for our simple overlay OBJ layout.
    first_attr_index = next(
        (index for index, line in enumerate(lines) if line.strip().startswith("ATTR_")),
        None,
    )
    if first_attr_index is None:
        raise ValueError(f"No ATTR section found in {source}")

    before_attr = lines[:first_attr_index]
    attr_and_after = lines[first_attr_index:]

    border_index_start = index_count

    generated = [
        *before_attr,
        "",
        "# XPTO positioning border vertices",
        *border_vertices,
        "",
        "# XPTO positioning border indices",
        *border_indices,
        "",
        *attr_and_after,
        "",
        "# XPTO magenta positioning border",
        "ATTR_draw_enable",
        "ATTR_no_cull",
        "ATTR_no_blend",
        "ATTR_diffuse_rgb 1.0 0.0 1.0",
        f"TRIS {border_index_start} {len(border_indices)}",
        "ATTR_diffuse_rgb 0.0 0.0 0.0",
    ]

    output.write_text("\n".join(generated) + "\n", encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate positioning-border variants of XPTO overlay OBJ files.")
    parser.add_argument("source", nargs="+", help="Source OBJ file(s).")
    parser.add_argument("--output", help="Output OBJ path. Only valid with one source.")
    parser.add_argument("--thickness", type=float, default=0.0020, help="Border thickness in OBJ units.")
    parser.add_argument("--z-offset", type=float, default=-0.0005, help="Small Z offset to avoid z-fighting.")
    args = parser.parse_args()

    sources = [Path(value) for value in args.source]

    if args.output and len(sources) != 1:
        print("--output can only be used with one source file.")
        return 1

    for source in sources:
        if not source.exists():
            print(f"Source OBJ not found: {source}")
            return 1

        output = Path(args.output) if args.output else make_output_path(source)
        generate_positioning_border(source, output, thickness=args.thickness, z_offset=args.z_offset)
        print(f"Wrote: {output}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())