import unreal

bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())
skm = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin")
print(f"SKM loaded: {skm}")
cdo.set_editor_property("LocomotionSkeletalMesh", skm)
print(f"Set LocomotionSkeletalMesh: {cdo.get_editor_property('LocomotionSkeletalMesh')}")

# Save
unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/BP_GraceCharacter")
print("Saved BP_GraceCharacter")
