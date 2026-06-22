import unreal

CUBE = unreal.load_asset("/Game/BackgroundBuildings/Textures/FakeInteriors/T_cubeFakeInterior2")
MAT_PATH = "/Game/Core/Materials"
MAT_NAME = "M_FakeWindowInterior"
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

full = MAT_PATH + "/" + MAT_NAME
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset(MAT_NAME, MAT_PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)  # self-lit fake room
mat.set_editor_property("two_sided", True)

# Cube sample driven by the camera->pixel view vector => a "room behind the glass" parallax look.
cubesample = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameterCube, -400, 0)
cubesample.set_editor_property("parameter_name", "InteriorCube")
cubesample.set_editor_property("texture", CUBE)

camvec = MEL.create_material_expression(mat, unreal.MaterialExpressionCameraVectorWS, -750, 60)

# brightness knob so the dev can dim/brighten the fake interior
mult = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 0)
bright = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 220)
bright.set_editor_property("parameter_name", "Brightness")
bright.set_editor_property("default_value", 1.0)

ok1 = MEL.connect_material_expressions(camvec, "", cubesample, "UVs")
ok2 = MEL.connect_material_expressions(cubesample, "RGB", mult, "A")
ok3 = MEL.connect_material_expressions(bright, "", mult, "B")
ok4 = MEL.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
print("connections:", ok1, ok2, ok3, ok4)

MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(full)
print("SAVED", full)

# PREVIEW: apply to a few floor-3 windows only, do NOT save the level (reversible).
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
applied = 0
windows_done = 0
for a in eas.get_all_level_actors():
    if windows_done >= 8:
        break
    lbl = a.get_actor_label()
    if "windowwall" not in lbl.lower():
        continue
    z = a.get_actor_location().z
    if not (887 <= z <= 1387):   # floor 3 only
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    c = comps[0]
    hit = False
    for slot in range(c.get_num_materials()):
        m = c.get_material(slot)
        if m and m.get_name() == "M_WindowGlass":
            c.set_material(slot, mat)
            applied += 1
            hit = True
    if hit:
        windows_done += 1
        print("  preview on:", lbl, "z=%.0f" % z)
print("PREVIEW applied to %d glass slots on %d floor-3 windows (level NOT saved)." % (applied, windows_done))
