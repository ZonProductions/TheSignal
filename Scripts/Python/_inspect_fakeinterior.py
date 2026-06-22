import unreal

# M_Building master — does it do interior mapping with the fake-interior cubes? what params?
m = unreal.load_asset("/Game/BackgroundBuildings/MasterMaterials/M_Building")
print("M_Building:", m)
if m:
    print(" domain:", m.get_editor_property("material_domain"))
    print(" blend:", m.get_editor_property("blend_mode"))
    # scalar/vector/texture params
    try:
        sp = unreal.MaterialEditingLibrary.get_scalar_parameter_names(m)
        tp = unreal.MaterialEditingLibrary.get_texture_parameter_names(m)
        vp = unreal.MaterialEditingLibrary.get_vector_parameter_names(m)
        print(" scalar params:", list(sp))
        print(" texture params:", list(tp))
        print(" vector params:", list(vp))
    except Exception as e:
        print(" param read err:", e)

# what does SM_WindowWall currently use?
print("\nSM_WindowWall materials:")
sm = unreal.load_asset("/Game/office_BigCompanyArchViz/Meshes/SM_WindowWall")
if not sm:
    # search
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","StaticMesh"), True):
        if str(ad.get_editor_property("asset_name")) == "SM_WindowWall":
            sm = ad.get_asset(); print("  found at", ad.get_editor_property("package_name")); break
if sm:
    mats = sm.get_editor_property("static_materials")
    for i, smat in enumerate(mats):
        mi = smat.get_editor_property("material_interface")
        print("  slot %d: %s" % (i, mi.get_name() if mi else None))

# Is there a reusable interior-mapping material function?
print("\nMaterial functions with 'interior'/'parallax':")
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","MaterialFunction"), True):
    n = str(ad.get_editor_property("asset_name")).lower()
    if "interior" in n or "parallax" in n or "roombox" in n or "cubemap" in n:
        print("  ", ad.get_editor_property("package_name"))
