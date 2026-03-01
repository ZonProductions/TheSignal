import unreal

# The asset registry found it at this path — load_asset needs the package path only
skm = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin')
if not skm:
    print("ERROR: Could not load SKM_UEFN_Mannequin")
else:
    skeleton = skm.get_editor_property('skeleton')
    print(f"Skeleton: {skeleton.get_path_name() if skeleton else 'NONE'}")

    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property('target_skeleton', skeleton)

    parent_class = unreal.load_class(None, '/Script/TheSignal.ZP_GraceAnimInstance')
    if parent_class:
        factory.set_editor_property('parent_class', parent_class)
        print(f"Parent class set: ZP_GraceAnimInstance")
    else:
        print("WARNING: Could not find ZP_GraceAnimInstance")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    abp = asset_tools.create_asset('ABP_GraceLocomotion', '/Game/Core/Player', None, factory)
    if abp:
        print(f"SUCCESS: Created {abp.get_path_name()}")
        unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/ABP_GraceLocomotion')
    else:
        print("ERROR: Failed to create AnimBlueprint")
