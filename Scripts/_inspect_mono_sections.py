import unreal
m = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono")
mats = m.get_editor_property("materials")
# Try to read LOD0 sections via the skeletal mesh's LOD render data is not exposed to Python;
# instead report the material slot list with index so we can map sections by material slot.
print("SLOTS:")
for i, s in enumerate(mats):
    print(i, str(s.material_slot_name))
# Section info: use the SkeletalMesh LOD model section -> material index mapping if available.
try:
    num_lods = m.get_lod_count()
    print("LOD_COUNT", num_lods)
except Exception as e:
    print("LODCOUNT_ERR", e)
