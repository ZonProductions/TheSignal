import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()

# cube texture class?
for i in (1,2,3):
    t = unreal.load_asset("/Game/BackgroundBuildings/Textures/FakeInteriors/T_cubeFakeInterior%d" % i)
    print("T_cubeFakeInterior%d:" % i, type(t).__name__ if t else None)

# what does M_Building's "CubeMap Interior" default to?
m = unreal.load_asset("/Game/BackgroundBuildings/MasterMaterials/M_Building")
try:
    v = unreal.MaterialEditingLibrary.get_material_default_texture_parameter_value(m, "CubeMap Interior")
    print("M_Building CubeMap Interior default:", v.get_name() if v else None, type(v).__name__ if v else "")
except Exception as e:
    print("cube param err:", e)

# find the glass: meshes/materials referencing M_WindowGlass, and any SM_*Glass*/SM_*WindowPane*
print("\nM_WindowGlass:", unreal.load_asset("/Game/Core/Materials/M_WindowGlass"))
print("Glass-ish static meshes (office pack):")
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","StaticMesh"), True):
    n = str(ad.get_editor_property("asset_name"))
    p = str(ad.get_editor_property("package_name"))
    if ("office_BigCompany" in p or "WindowWall" in n) and ("glass" in n.lower() or "window" in n.lower()):
        print("  ", n, "|", p)

# In the level: a SM_WindowWall actor — list ALL its slot materials incl. which is glass-like
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if "windowwall" in a.get_actor_label().lower():
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps: continue
        c = comps[0]
        print("\nLevel actor", a.get_actor_label(), "materials:")
        for i in range(c.get_num_materials()):
            mat = c.get_material(i)
            print("  slot %d: %s" % (i, mat.get_name() if mat else None))
        break
