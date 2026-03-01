import unreal

# Hidden Mesh skeleton
uefn = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin")
uefn_sk = uefn.get_editor_property("skeleton") if uefn else None
print(f"Hidden Mesh (UEFN): {uefn_sk.get_path_name() if uefn_sk else 'NONE'}")

# PlayerMesh skeleton (Kinemation operator)
operator = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Characters/Operator/SKM_Operator_Mono")
if not operator:
    # Try alternate paths
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    all_assets = ar.get_all_assets()
    for a in all_assets:
        if "Operator" in str(a.asset_name) and "SKM" in str(a.asset_name):
            print(f"  Found: {a.package_name}/{a.asset_name}")
            operator = unreal.load_asset(str(a.package_name))
            break

if operator:
    op_sk = operator.get_editor_property("skeleton")
    print(f"PlayerMesh (Operator): {op_sk.get_path_name() if op_sk else 'NONE'}")
    if uefn_sk and op_sk:
        print(f"Same skeleton? {uefn_sk.get_path_name() == op_sk.get_path_name()}")
else:
    print("PlayerMesh skeleton: NOT FOUND")
