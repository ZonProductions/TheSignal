import unreal

mesh = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Steps")
if mesh:
    body = mesh.get_editor_property("body_setup")
    if body:
        old = body.get_editor_property("collision_trace_flag")
        print(f"SM_Steps current collision: {old}")
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        print("Set to CTF_USE_COMPLEX_AS_SIMPLE")
        unreal.EditorAssetLibrary.save_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Steps")
        print("Saved.")
    else:
        print("No BodySetup found")
else:
    print("Could not load SM_Steps")
