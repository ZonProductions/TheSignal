import unreal

bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
if not bp:
    print("ERROR: BP_GraceCharacter not found")
else:
    cdo = unreal.get_default_object(bp.generated_class())

    # 1. Set AnimBP on PlayerMesh
    mesh = cdo.get_editor_property('PlayerMesh')
    if mesh:
        abp = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Animations/UE5/ABP_TacticalShooter_UE5')
        if abp:
            mesh.set_editor_property('AnimClass', abp.generated_class())
            print(f"OK: PlayerMesh AnimClass set to {abp.get_name()}")
        else:
            print("ERROR: ABP_TacticalShooter_UE5 not found")
    else:
        print("ERROR: PlayerMesh not found on CDO")

    # 2. Set WeaponClass = BP_TR15
    weapon_bp = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Blueprints/Weapons/BP_TR15')
    if weapon_bp:
        cdo.set_editor_property('WeaponClass', weapon_bp.generated_class())
        print(f"OK: WeaponClass set to {weapon_bp.get_name()}")
    else:
        print("ERROR: BP_TR15 not found")

    # 3. Save
    unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
    print("OK: BP_GraceCharacter saved")
