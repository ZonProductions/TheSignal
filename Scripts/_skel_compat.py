import unreal
def bones(skpath):
    sk = unreal.load_asset(skpath)
    if not sk: return None
    try: return [str(b) for b in sk.get_bone_names()] if hasattr(sk,"get_bone_names") else None
    except Exception: return None
ccmh = unreal.load_asset("/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Skeleton")
manny = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Demo/Manny/UE5/SKM_Manny_Skeleton")
# bone names not enumerable directly; use the mesh's reference skeleton via editor props
def mesh_bones(mpath):
    m = unreal.load_asset(mpath)
    n = m.num_bones() if hasattr(m,"num_bones") else None
    names = []
    if hasattr(m,"get_bone_name"):
        for i in range(min(n or 0, 400)):
            names.append(str(m.get_bone_name(i)))
    return n, names
nb_c, names_c = mesh_bones("/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Body_Male")
nb_m, names_m = mesh_bones("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono")
print("CCMH_Body_Male bones:", nb_c)
print("Operator(Manny) bones:", nb_m)
setc, setm = set(names_c), set(names_m)
common = setc & setm
print("common bones:", len(common), "/ CCMH", len(setc), "Manny", len(setm))
print("Manny bones NOT in CCMH:", sorted(list(setm-setc))[:20])
print("first 15 CCMH:", names_c[:15])
print("=== API check ===")
print("IKRetargetBatchOperation:", hasattr(unreal,"IKRetargetBatchOperation"))
if hasattr(unreal,"IKRetargetBatchOperation"):
    print([m for m in dir(unreal.IKRetargetBatchOperation) if not m.startswith("_")])
