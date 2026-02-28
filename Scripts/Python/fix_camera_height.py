import unreal

bp_path = '/Game/Player/Blueprints/BP_GraceTactical'
bp = unreal.load_asset(bp_path)

# Access SCS via get_editor_property
scs = bp.get_editor_property('simple_construction_script')
print(f"SCS: {scs}")

for node in scs.get_all_nodes():
    comp = node.component_template
    name = comp.get_name()
    if isinstance(comp, unreal.CameraComponent):
        loc = comp.get_editor_property('relative_location')
        print(f"Found camera '{name}' at relative_location={loc}")
        comp.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, 76.0))
        new_loc = comp.get_editor_property('relative_location')
        print(f"Set '{name}' to relative_location={new_loc}")

unreal.EditorAssetLibrary.save_loaded_asset(bp)
print("Saved")
