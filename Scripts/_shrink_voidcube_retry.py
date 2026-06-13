import unreal
sm = unreal.load_asset("/Game/HorrorLight/SM_VoidCube")
smes = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

# Inspect nanite + LOD count
try:
    ns = sm.get_editor_property("nanite_settings")
    print("Nanite enabled before:", ns.get_editor_property("enabled"))
    ns.set_editor_property("enabled", False)
    sm.set_editor_property("nanite_settings", ns)
except Exception as e:
    print("nanite read/set err:", str(e)[:60])

print("LOD count:", smes.get_lod_count(sm))

# Re-apply build scale
bs = smes.get_lod_build_settings(sm, 0)
bs.build_scale3d = unreal.Vector(0.02, 0.02, 0.02)
smes.set_lod_build_settings(sm, 0, bs)

unreal.EditorAssetLibrary.save_asset("/Game/HorrorLight/SM_VoidCube")

# Reload + verify
sm2 = unreal.load_asset("/Game/HorrorLight/SM_VoidCube")
bs2 = smes.get_lod_build_settings(sm2, 0)
print("build_scale3d after:", bs2.build_scale3d)
print("bounds extent after:", sm2.get_bounds().box_extent)
print("verts:", smes.get_number_verts(sm2, 0))
