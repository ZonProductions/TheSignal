import unreal

mi = unreal.load_asset('/Game/TheSignal/Materials/MI_SignCover')
print(f'MI_SignCover: {mi}')
if mi:
    print(f'  Class: {mi.get_class().get_name()}')
    print(f'  Parent: {mi.get_editor_property("parent").get_name()}')

# Load the BP and set the material on the CDO's mesh component
bp = unreal.load_asset('/Game/TheSignal/Blueprints/LevelDesign/BP_SignCover')
print(f'BP: {bp}')
if bp:
    cdo = unreal.get_default_object(bp.generated_class())
    print(f'CDO: {cdo}')
    # Find the CoverMesh component on the CDO
    comps = cdo.get_components_by_class(unreal.StaticMeshComponent)
    for c in comps:
        print(f'  Comp: {c.get_name()} mesh={c.static_mesh}')
        if mi:
            c.set_material(0, mi)
            print(f'  Set material to MI_SignCover')

    # Save
    unreal.EditorAssetLibrary.save_asset('/Game/TheSignal/Blueprints/LevelDesign/BP_SignCover')
    print('Saved BP')
