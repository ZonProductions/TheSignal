import unreal
m = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono")
mats = m.get_editor_property("materials")
print("NUM_MATERIALS", len(mats))
for i, s in enumerate(mats):
    name = s.material_interface.get_name() if s.material_interface else None
    print(i, s.material_slot_name, name)
