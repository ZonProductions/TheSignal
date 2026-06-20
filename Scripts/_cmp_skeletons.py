import unreal

def info(path):
    a = unreal.load_asset(path)
    if not a:
        print("MISSING", path); return None
    sk = a.get_editor_property("skeleton") if hasattr(a, "get_editor_property") else None
    try:
        sk = a.skeleton
    except Exception:
        pass
    skn = sk.get_path_name() if sk else "None"
    print("MESH", path.split("/")[-1], "-> SKEL", skn)
    return sk

paths = [
    "/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono",
    "/Game/CharacterCustomizer/Characters/ApparelPack_Example_Character/CCMH_Body_Male",
    "/Game/CharacterCustomizer/Characters/ApparelPack_Example_Character/m_tal_nrw_body",
]
skels = {}
for p in paths:
    s = info(p)
    if s:
        skels[p] = s

# Bone count + first 30 bone names for each distinct skeleton
seen = set()
for p, s in skels.items():
    spath = s.get_path_name()
    if spath in seen:
        continue
    seen.add(spath)
    bones = s.get_editor_property("bone_tree") if False else None
    try:
        names = [str(n) for n in s.get_bone_names()] if hasattr(s, "get_bone_names") else None
    except Exception:
        names = None
    print("SKEL", spath)
    if names:
        print("  NUM_BONES", len(names))
        print("  FIRST30", names[:30])
    else:
        print("  (bone names not enumerable via this API)")
