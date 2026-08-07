"""
The Signal - make pack wall collision DOUBLE-SIDED.

ResearchMegaPack meshes ship with single-sided complex collision (double_sided_geometry=False
on the BodySetup) while the project runs them CTF_USE_COMPLEX_AS_SIMPLE. A line trace only hits
triangles from the side they face, so whether a wall "exists" for audio occlusion / peek / AI
sight depends on which way the artist faced it. Measured 2026-08-07 on the sub-basement divider:
player->shambler ray hits, shambler->player ray passes clean through the same wall.

This sweep flips double_sided_geometry=True on every complex-as-simple StaticMesh under the
pack root. Sibling of Scripts/fix_all_generators2.py (the convex-hull remediation) — run both
on any new purchased level.

Run via the MCP python endpoint / nwiro execute_python with the editor open, then save.
"""

import unreal

PACK_ROOTS = ["/Game/ResearchMegaPack"]

ar = unreal.AssetRegistryHelpers.get_asset_registry()
f = unreal.ARFilter(class_names=["StaticMesh"], recursive_paths=True, package_paths=PACK_ROOTS)

changed = []
skipped_ds = 0
skipped_flag = 0
errors = []

for a in ar.get_assets(f):
    path = str(a.package_name)
    m = unreal.load_asset(path)
    if not m:
        errors.append(path + " (load failed)")
        continue
    bs = m.get_editor_property("body_setup")
    if not bs:
        errors.append(path + " (no body setup)")
        continue
    if bs.get_editor_property("collision_trace_flag") != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
        skipped_flag += 1
        continue
    if bs.get_editor_property("double_sided_geometry"):
        skipped_ds += 1
        continue
    bs.set_editor_property("double_sided_geometry", True)
    m.modify()
    changed.append(path.split("/")[-1])

print("[DoubleSided] changed=%d alreadyDS=%d notComplexAsSimple=%d errors=%d"
      % (len(changed), skipped_ds, skipped_flag, len(errors)))
for n in changed:
    print("   +", n)
for e in errors[:10]:
    print("   ERR", e)

if changed:
    ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    print("[DoubleSided] save_dirty_packages:", ok)
