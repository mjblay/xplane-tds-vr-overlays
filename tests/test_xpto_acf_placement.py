from argparse import Namespace
from pathlib import Path

import pytest

from tools.xpto import (
    find_overlay_matches,
    patch_overlay_placement_text,
    set_overlay_placement,
    unique_overlay_match,
)


ACF_TEMPLATE = """I
800 Version
P _obja/12/_obj_flags 9
P _obja/12/_v10_att_file_stl TDS_Overlay/TDS_GTN750_ScreenOnly_U1_F33A.obj
P _obja/12/_v10_att_phi_ref 0.000000000
P _obja/12/_v10_att_psi_ref 1.000000000
P _obja/12/_v10_att_the_ref 2.000000000
P _obja/12/_v10_att_x_acf_prt_ref 3.000000000
P _obja/12/_v10_att_y_acf_prt_ref 4.000000000
P _obja/12/_v10_att_z_acf_prt_ref 5.000000000
P unrelated/value keep-me
P _obja/count 13
"""


def write_acf(path: Path, text: str = ACF_TEMPLATE) -> Path:
    path.write_text(text, encoding="utf-8")
    return path


def test_unique_target_detection_finds_overlay(tmp_path: Path) -> None:
    acf = write_acf(tmp_path / "Example.acf")

    matches = find_overlay_matches([acf], "gtn750-u1")

    assert len(matches) == 1
    assert matches[0].obj.index == 12
    assert matches[0].obj.file_stl == "TDS_Overlay/TDS_GTN750_ScreenOnly_U1_F33A.obj"


def test_unique_target_detection_refuses_ambiguous_matches(tmp_path: Path) -> None:
    first = write_acf(tmp_path / "First.acf")
    second = write_acf(tmp_path / "Second.acf")

    with pytest.raises(ValueError, match="ambiguous"):
        unique_overlay_match([first, second], "gtn750-u1")


def test_set_overlay_placement_dry_run_does_not_write(tmp_path: Path) -> None:
    aircraft = tmp_path / "Aircraft"
    aircraft.mkdir()
    acf = write_acf(aircraft / "Example.acf")
    original = acf.read_text(encoding="utf-8")

    result = set_overlay_placement(
        Namespace(
            aircraft=str(aircraft),
            acf=None,
            target="gtn750-u1",
            x=10.0,
            y=20.0,
            z=30.0,
            pitch=None,
            yaw=None,
            roll=None,
            apply=False,
            backup_root=None,
        )
    )

    assert result == 0
    assert acf.read_text(encoding="utf-8") == original


def test_set_overlay_placement_apply_requires_backup_root(tmp_path: Path) -> None:
    aircraft = tmp_path / "Aircraft"
    aircraft.mkdir()
    acf = write_acf(aircraft / "Example.acf")
    original = acf.read_text(encoding="utf-8")

    result = set_overlay_placement(
        Namespace(
            aircraft=str(aircraft),
            acf=None,
            target="gtn750-u1",
            x=10.0,
            y=20.0,
            z=30.0,
            pitch=None,
            yaw=None,
            roll=None,
            apply=True,
            backup_root=None,
        )
    )

    assert result == 1
    assert acf.read_text(encoding="utf-8") == original


def test_patch_overlay_placement_updates_xyz_and_preserves_unrelated_content(tmp_path: Path) -> None:
    acf = write_acf(tmp_path / "Example.acf")
    match = unique_overlay_match([acf], "gtn750-u1")

    patched = patch_overlay_placement_text(
        acf.read_text(encoding="utf-8"),
        match,
        x=1.25,
        y=-2.5,
        z=3.75,
        pitch=4.0,
        yaw=5.0,
        roll=6.0,
    )

    assert "P _obja/12/_v10_att_x_acf_prt_ref 1.250000000" in patched
    assert "P _obja/12/_v10_att_y_acf_prt_ref -2.500000000" in patched
    assert "P _obja/12/_v10_att_z_acf_prt_ref 3.750000000" in patched
    assert "P _obja/12/_v10_att_the_ref 4.000000000" in patched
    assert "P _obja/12/_v10_att_psi_ref 5.000000000" in patched
    assert "P _obja/12/_v10_att_phi_ref 6.000000000" in patched
    assert "P unrelated/value keep-me" in patched
    assert "P _obja/count 13" in patched


def test_patch_overlay_placement_updates_obj_xyz_syntax(tmp_path: Path) -> None:
    acf = write_acf(
        tmp_path / "Modern.acf",
        """I
800 Version
P _obja/2/_obj_flags 9
P _obja/2/_v10_att_file_stl TDS_Overlay/TDS_GTN650_ScreenOnly_U2_PAE_A36.obj
P _obja/2/_obj_xyz 0.000000000 0.000000000 0.000000000
P _obja/2/_obj_psi 0.000000000
P _obja/2/_obj_the 0.000000000
P _obja/2/_obj_phi 0.000000000
P _obja/count 3
""",
    )
    match = unique_overlay_match([acf], "gtn650-u2")

    patched = patch_overlay_placement_text(
        acf.read_text(encoding="utf-8"),
        match,
        x=7.0,
        y=8.0,
        z=9.0,
        pitch=1.0,
        yaw=2.0,
        roll=3.0,
    )

    assert "P _obja/2/_obj_xyz 7.000000000 8.000000000 9.000000000" in patched
    assert "P _obja/2/_obj_the 1.000000000" in patched
    assert "P _obja/2/_obj_psi 2.000000000" in patched
    assert "P _obja/2/_obj_phi 3.000000000" in patched
