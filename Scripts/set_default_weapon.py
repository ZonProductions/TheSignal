import unreal

bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())

# Load the Viper pistol Blueprint class
viper = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Blueprints/Weapons/BP_WK-11_Viper")
if viper:
    viper_class = viper.generated_class()
    cdo.set_editor_property("WeaponClass", viper_class)
    unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/BP_GraceCharacter")
    print(f"Set WeaponClass to: {viper_class.get_name()}")
else:
    print("ERROR: Could not load BP_WK-11_Viper")
