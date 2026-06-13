import unreal
sm = unreal.load_asset("/Game/HorrorLight/SM_VoidCube")
smes = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
bs = smes.get_lod_build_settings(sm, 0)
print("build_scale3d persisted as:", bs.build_scale3d)
b = sm.get_bounds()
print("bounds extent:", b.box_extent)
# Is there a rebuild / nanite / source available?
print("num verts LOD0:", smes.get_number_verts(sm, 0))
