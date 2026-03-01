import unreal

bs = unreal.load_asset("/Game/Core/Player/BS_Grace_Locomotion")
if bs:
    params = bs.get_editor_property("blend_parameters")
    bp = params[0]
    print(f"Before: min={bp.get_editor_property('min')}, max={bp.get_editor_property('max')}")

    bp.set_editor_property("min", 0.0)
    bp.set_editor_property("max", 600.0)
    bp.set_editor_property("display_name", "Speed")
    params[0] = bp
    bs.set_editor_property("blend_parameters", params)

    # Verify
    params2 = bs.get_editor_property("blend_parameters")
    bp2 = params2[0]
    print(f"After: min={bp2.get_editor_property('min')}, max={bp2.get_editor_property('max')}")

    unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/BS_Grace_Locomotion")
    print("Saved BS_Grace_Locomotion")
