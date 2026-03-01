import unreal

# PlayerMesh uses SKM_Operator_Mono — get its skeleton
operator = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono")
skeleton = operator.get_editor_property("skeleton") if operator else None
print(f"Operator skeleton: {skeleton.get_path_name() if skeleton else 'NONE'}")

if not skeleton:
    print("ERROR: Could not find Operator skeleton")
else:
    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("target_skeleton", skeleton)

    parent_class = unreal.load_class(None, "/Script/TheSignal.ZP_GracePlayerAnimInstance")
    if parent_class:
        factory.set_editor_property("parent_class", parent_class)
        print(f"Parent class: ZP_GracePlayerAnimInstance")
    else:
        print("WARNING: Could not find ZP_GracePlayerAnimInstance")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    abp = asset_tools.create_asset("ABP_GracePlayer", "/Game/Core/Player", None, factory)
    if abp:
        print(f"SUCCESS: Created {abp.get_path_name()}")
        unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/ABP_GracePlayer")
    else:
        print("ERROR: Failed to create AnimBlueprint")
