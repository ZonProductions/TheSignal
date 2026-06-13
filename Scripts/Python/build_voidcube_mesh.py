"""Create SM_VoidCube: a very small cube static mesh with the M_VoidCube
black-glow material, for use as the Niagara Mesh Renderer particle mesh.
Shrinks the engine Cube (100cm) to ~2cm via LOD BuildScale3D."""
import unreal

EAL = unreal.EditorAssetLibrary
SRC = "/Engine/BasicShapes/Cube"
DST = "/Game/HorrorLight/SM_VoidCube"
MAT = "/Game/HorrorLight/M_VoidCube"
SCALE = 0.02  # 100cm * 0.02 = 2cm cube

if EAL.does_asset_exist(DST):
    EAL.delete_asset(DST)
EAL.duplicate_asset(SRC, DST)
sm = unreal.load_asset(DST)
print("Duplicated SM_VoidCube:", sm is not None)

# Shrink the geometry via build scale, then rebuild
smes = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
bs = smes.get_lod_build_settings(sm, 0)
bs.build_scale3d = unreal.Vector(SCALE, SCALE, SCALE)
smes.set_lod_build_settings(sm, 0, bs)
print("Set BuildScale3D:", SCALE, "(", 100 * SCALE, "cm cube )")

# Assign the void-glow material
mat = unreal.load_asset(MAT)
sm.set_material(0, mat)

EAL.save_asset(DST)
b = sm.get_bounds()
print("SM_VoidCube saved. Bounds extent (cm):", b.box_extent,
      "| material:", sm.get_material(0).get_name() if sm.get_material(0) else None)
