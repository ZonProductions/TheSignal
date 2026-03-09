import unreal

# Check MI_FloorSign_1 vs MI_FloorSign_2 — are they actually different?
for i in [1, 2]:
    mi = unreal.load_asset(f"/Game/TheSignal/Materials/MI_FloorSign_{i}")
    if mi:
        overrides = mi.get_editor_property("texture_parameter_values")
        print(f"MI_FloorSign_{i}: {len(overrides) if overrides else 0} texture overrides")
        for ov in (overrides or []):
            info = ov.get_editor_property("parameter_info")
            val = ov.get_editor_property("parameter_value")
            name = info.get_editor_property("name")
            print(f"  '{name}' = {val.get_path_name() if val else 'NONE'}")

        scalar_overrides = mi.get_editor_property("scalar_parameter_values")
        for ov in (scalar_overrides or []):
            info = ov.get_editor_property("parameter_info")
            val = ov.get_editor_property("parameter_value")
            name = info.get_editor_property("name")
            print(f"  scalar '{name}' = {val}")

# Test: manually set material on a placed actor to verify SetMaterial works
actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if "FloorSign" in a.get_actor_label():
        mi2 = unreal.load_asset("/Game/TheSignal/Materials/MI_FloorSign_3")
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if comps and mi2:
            comps[0].set_material(0, mi2)
            print(f"\nForce-set {a.get_actor_label()} material to MI_FloorSign_3")
        break
