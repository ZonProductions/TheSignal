import unreal

# Try loading the class directly
bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
if bp_class:
    print(f"Class loaded: {bp_class}")
else:
    print("Could not load class, trying blueprint...")
    bp = unreal.load_asset('/Game/Player/Blueprints/BP_GraceTactical')
    print(f"Blueprint: {bp}")
    if bp:
        gen_class = bp.get_editor_property('generated_class')
        print(f"Generated class: {gen_class}")
        if gen_class:
            cdo = unreal.get_default_object(gen_class)
            print(f"CDO: {cdo}")
            if cdo:
                comps = cdo.get_components_by_class(unreal.CameraComponent)
                print(f"Camera components: {len(comps)}")
                for c in comps:
                    print(f"  {c.get_name()} rel_loc={c.relative_location}")
