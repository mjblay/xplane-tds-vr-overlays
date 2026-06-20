from pathlib import Path

import pytest

from tools.xpto import find_xplane_root


def test_find_xplane_root_by_exe(tmp_path: Path) -> None:
    xplane = tmp_path / "X-Plane 12"
    aircraft = xplane / "Aircraft" / "Example Aircraft"
    aircraft.mkdir(parents=True)
    (xplane / "X-Plane.exe").write_text("", encoding="utf-8")

    assert find_xplane_root(aircraft) == xplane


def test_find_xplane_root_by_resources_and_aircraft(tmp_path: Path) -> None:
    xplane = tmp_path / "X-Plane 12"
    aircraft = xplane / "Aircraft" / "Example Aircraft"
    resources = xplane / "Resources"

    aircraft.mkdir(parents=True)
    resources.mkdir(parents=True)

    assert find_xplane_root(aircraft) == xplane


def test_find_xplane_root_returns_none_when_not_xplane_tree(tmp_path: Path) -> None:
    aircraft = tmp_path / "Aircraft" / "Example Aircraft"
    aircraft.mkdir(parents=True)

    assert find_xplane_root(aircraft) is None