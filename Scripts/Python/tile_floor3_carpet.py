import unreal
eas  = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les  = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL  = unreal.MaterialEditingLibrary
EAL  = unreal.EditorAssetLibrary

SQUARE = 100.0          # 1m carpet square (matches the map's 1x1m floor grid)
DEST   = "/Game/Office/OfficeMats"
PLAN   = {  # current material name -> (parent path, new tiled MI name)
    "MI_Carpet02": ("/Game/Office/OfficeMats/MI_Carpet02.MI_Carpet02", "MI_Carpet02_Tile1m"),
    "MI_Carpet03": ("/Game/Office/OfficeMats/MI_Carpet03.MI_Carpet03", "MI_Carpet03_Tile1m"),
}

def get_or_make_mic(parent_path, name):
    full = DEST + "/" + name
    if EAL.does_asset_exist(full):
        return unreal.load_asset(full)              # update in place (no overwrite)
    parent = unreal.load_asset(parent_path)
    mic = tools.create_asset(name, DEST, unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew())
    mic.set_editor_property("parent", parent)
    return mic

# find floor-3 carpet plates
plates = []
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    c = cs[0]
    sm = c.get_editor_property("static_mesh")
    if not sm: continue
    org, ext = a.get_actor_bounds(False)
    if ext.z > 80: continue
    for i in range(c.get_num_materials()):
        m = c.get_material(i)
        if m and m.get_name() in PLAN:
            plates.append((a, c, i, m.get_name(), ext.x*2, ext.y*2))
            break

print("carpet plates found:", len(plates))
for a, c, slot, matname, fx, fy in plates:
    tilU = round(fx / SQUARE, 3)
    tilV = round(fy / SQUARE, 3)
    parent_path, newname = PLAN[matname]
    mic = get_or_make_mic(parent_path, newname)
    MEL.set_material_instance_vector_parameter_value(mic, "Tilling",
            unreal.LinearColor(tilU, tilV, 0.0, 0.0))
    EAL.save_asset(DEST + "/" + newname)
    c.set_material(slot, mic)
    # verify
    chk = c.get_material(slot)
    print("  %-22s slot%d  %s -> %s  Tilling=(%.2f,%.2f)  (squares of %.0fUU)" % (
        a.get_actor_label(), slot, matname, chk.get_name(), tilU, tilV, SQUARE))

print("level saved:", les.save_current_level())
