"""Narrow Grace's capsule via the runtime data asset.

DA_GraceMovement_Default is what ZP_GraceCharacter pulls via SetCapsuleSize
at line 265 of ZP_GraceCharacter.cpp. Changing the .uasset's CapsuleRadius
property is what actually shifts the runtime collision capsule. The C++
header value (55.0f) is just the seed default for NEW instances and doesn't
affect this existing asset.
"""
import unreal

ASSET_PATH = "/Game/Core/Data/DA_GraceMovement_Default"
NEW_RADIUS = 34.0  # UE Character default
OLD_RADIUS_EXPECTED = 55.0

asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not asset:
    print(f"FAIL: could not load {ASSET_PATH}")
    raise SystemExit

prev = asset.get_editor_property("CapsuleRadius")
half_h = asset.get_editor_property("CapsuleHalfHeight")
crouch_h = asset.get_editor_property("CrouchedHalfHeight")
mesh_z = asset.get_editor_property("PlayerMeshOffsetZ")
print(f"BEFORE  CapsuleRadius={prev}  CapsuleHalfHeight={half_h}  CrouchedHalfHeight={crouch_h}  PlayerMeshOffsetZ={mesh_z}")

if abs(prev - OLD_RADIUS_EXPECTED) > 0.01:
    print(f"WARN: expected radius {OLD_RADIUS_EXPECTED} but found {prev}. Proceeding anyway.")

asset.set_editor_property("CapsuleRadius", NEW_RADIUS)
after = asset.get_editor_property("CapsuleRadius")
print(f"AFTER   CapsuleRadius={after}")

# Persist to disk so the change survives editor restart
ok = unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False)
print(f"SAVE    ok={ok}")

print("\nDone. Restart PIE (no C++ rebuild needed — DataAsset only) and test the stairwell.")
