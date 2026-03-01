import unreal

bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())

# Get PlayerMesh component from CDO
player_mesh = cdo.get_editor_property("PlayerMesh")
print(f"PlayerMesh: {player_mesh}")

if player_mesh:
    # Load ABP_GracePlayer class
    abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
    if abp:
        anim_class = abp.generated_class()
        player_mesh.set_editor_property("AnimClass", anim_class)
        print(f"Set AnimClass: {player_mesh.get_editor_property('AnimClass')}")
    else:
        print("ERROR: Could not load ABP_GracePlayer")

unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/BP_GraceCharacter")
print("Saved BP_GraceCharacter")
