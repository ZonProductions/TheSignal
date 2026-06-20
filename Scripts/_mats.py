import unreal
def mats(p):
    m = unreal.load_asset(p)
    if not m: return p, "MISSING", []
    sm = m.get_editor_property("materials") if hasattr(m,"get_editor_property") else None
    out=[]
    try:
        for i,s in enumerate(sm):
            mi = s.material_interface
            out.append((i, str(s.material_slot_name), mi.get_path_name() if mi else None))
    except Exception as e: out=[("err",str(e),"")]
    return p.split("/")[-1], "OK", out
for p in ["/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono",
          "/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Body_Male",
          "/Game/CharacterCustomizer/Characters/CCMH/Apparel/ApparelPack_01/Male/UpperLowerBody/Overalls_01/Overalls_01"]:
    n,st,o = mats(p)
    print("===",n,st)
    for row in o: print("   ",row)
