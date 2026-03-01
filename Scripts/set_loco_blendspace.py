import unreal

bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())

# Set LocomotionBlendSpace
bs = unreal.load_asset("/Game/Core/Player/BS_Grace_Locomotion")
cdo.set_editor_property("LocomotionBlendSpace", bs)
print(f"LocomotionBlendSpace: {cdo.get_editor_property('LocomotionBlendSpace')}")

# Clear old LocomotionAnimClass (no longer used)
try:
    cdo.set_editor_property("LocomotionAnimClass", None)
    print("Cleared LocomotionAnimClass")
except:
    print("LocomotionAnimClass already clear or removed")

# Verify LocomotionSkeletalMesh still set
skm = cdo.get_editor_property("LocomotionSkeletalMesh")
print(f"LocomotionSkeletalMesh: {skm}")

unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/BP_GraceCharacter")
print("Saved BP_GraceCharacter")
