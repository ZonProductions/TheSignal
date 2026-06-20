import unreal
skel = unreal.load_asset("/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Skeleton")
print("skeleton:", skel)
f = unreal.AnimBlueprintFactory()
f.set_editor_property("target_skeleton", skel)
try:
    f.set_editor_property("parent_class", unreal.ZP_MeleeHandsAnimInstance)
except Exception as e:
    print("parent_class set err:", e)
tools = unreal.AssetToolsHelpers.get_asset_tools()
abp = tools.create_asset("ABP_MarcusMeleeHands", "/Game/Marcus", unreal.AnimBlueprint, f)
print("created:", abp)
if abp:
    pc = abp.get_editor_property("parent_class") if hasattr(abp,"get_editor_property") else None
    print("parent class:", pc)
    unreal.EditorAssetLibrary.save_asset(abp.get_path_name())
    print("saved")
