import unreal
m = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono.SKM_Operator_Mono')
out = []
for i, mat in enumerate(m.materials):
    mi = mat.material_interface
    out.append("{}: slot='{}' mat='{}'".format(i, mat.material_slot_name, mi.get_name() if mi else 'None'))
print("SKM_Operator_Mono material slots:\n" + "\n".join(out))
