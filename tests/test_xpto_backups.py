from pathlib import Path

from tools.xpto import validate_external_backup_root


def test_backup_root_outside_xplane_is_allowed(tmp_path: Path) -> None:
    xplane = tmp_path / "X-Plane 12"
    aircraft = xplane / "Aircraft" / "PAE-A36-XPTO-TEST"
    backup_root = tmp_path / "XPTO-Aircraft-Backups"

    aircraft.mkdir(parents=True)
    backup_root.mkdir()
    (xplane / "X-Plane.exe").write_text("", encoding="utf-8")

    assert validate_external_backup_root(aircraft, backup_root) is None


def test_backup_root_inside_aircraft_is_rejected(tmp_path: Path) -> None:
    xplane = tmp_path / "X-Plane 12"
    aircraft = xplane / "Aircraft" / "PAE-A36-XPTO-TEST"
    backup_root = aircraft / "TDS_Overlay_Backups"

    backup_root.mkdir(parents=True)
    (xplane / "X-Plane.exe").write_text("", encoding="utf-8")

    error = validate_external_backup_root(aircraft, backup_root)

    assert error is not None
    assert "aircraft folder" in error


def test_backup_root_equal_to_aircraft_is_rejected(tmp_path: Path) -> None:
    xplane = tmp_path / "X-Plane 12"
    aircraft = xplane / "Aircraft" / "PAE-A36-XPTO-TEST"

    aircraft.mkdir(parents=True)
    (xplane / "X-Plane.exe").write_text("", encoding="utf-8")

    error = validate_external_backup_root(aircraft, aircraft)

    assert error is not None
    assert "aircraft folder" in error


def test_backup_root_inside_xplane_is_rejected(tmp_path: Path) -> None:
    xplane = tmp_path / "X-Plane 12"
    aircraft = xplane / "Aircraft" / "PAE-A36-XPTO-TEST"
    backup_root = xplane / "Aircraft"

    aircraft.mkdir(parents=True)
    (xplane / "X-Plane.exe").write_text("", encoding="utf-8")

    error = validate_external_backup_root(aircraft, backup_root)

    assert error is not None
    assert "X-Plane folder" in error