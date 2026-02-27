"""
Asset Naming Convention Validator for The Signal
Run inside UE5 via Python Editor Script Plugin or standalone for file-based checks.

Usage (in UE5 Python console):
    exec(open('/Scripts/Python/validate_naming.py').read())

Usage (standalone from project root):
    python Scripts/Python/validate_naming.py
"""

import os
import re
import sys

# Naming convention rules: prefix -> asset types (folder hints or extensions)
PREFIXES = {
    "BP_": "Blueprint Actor",
    "BPC_": "Blueprint Component",
    "BPI_": "Blueprint Interface",
    "ABP_": "Animation Blueprint",
    "DA_": "Data Asset",
    "DT_": "Data Table",
    "WBP_": "Widget Blueprint",
    "SS_": "Subsystem",
    "E_": "Enumeration",
    "S_": "Structure",
    "M_": "Material",
    "MI_": "Material Instance",
    "T_": "Texture",
    "SM_": "Static Mesh",
    "SK_": "Skeletal Mesh",
    "SFX_": "Sound Effect",
    "MS_": "MetaSound Source",
    "SQ_": "Level Sequence",
    "MAP_": "Map/Level",
    "GI_": "Game Instance",
    "GM_": "Game Mode",
    "PC_": "Player Controller",
    "DEP_": "Deprecated Asset",
}

def validate_uasset_naming(content_dir: str) -> list[dict]:
    """Walk Content directory and flag .uasset files without a recognized prefix."""
    violations = []

    if not os.path.isdir(content_dir):
        print(f"[ERROR] Content directory not found: {content_dir}")
        return violations

    for root, dirs, files in os.walk(content_dir):
        # Skip deprecated folder
        if "_DEPRECATED" in root:
            continue

        for f in files:
            if not f.endswith(".uasset"):
                continue

            name = os.path.splitext(f)[0]
            has_valid_prefix = any(name.startswith(prefix) for prefix in PREFIXES)

            if not has_valid_prefix:
                rel_path = os.path.relpath(os.path.join(root, f), content_dir)
                violations.append({
                    "file": rel_path,
                    "asset_name": name,
                    "issue": "Missing or unrecognized naming prefix",
                })

    return violations


def main():
    # Determine Content directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_dir))
    content_dir = os.path.join(project_root, "Content")

    print(f"[validate_naming] Scanning: {content_dir}")
    violations = validate_uasset_naming(content_dir)

    if not violations:
        print("[validate_naming] All assets follow naming conventions.")
    else:
        print(f"[validate_naming] Found {len(violations)} violation(s):")
        for v in violations:
            print(f"  VIOLATION: {v['file']} — {v['issue']}")

    return violations


if __name__ == "__main__":
    main()
