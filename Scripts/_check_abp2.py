import unreal
abp = unreal.load_asset("/Game/Marcus/ABP_MarcusMeleeHands")
gc = abp.generated_class()
cdo = unreal.get_default_object(gc)
print("CDO class:", cdo.get_class().get_name(), "| is ZP_MeleeHands:", isinstance(cdo, unreal.ZP_MeleeHandsAnimInstance))
try:
    unreal.BlueprintEditorLibrary.reparent_blueprint(abp, unreal.ZP_MeleeHandsAnimInstance)
    print("reparent called")
except Exception as e:
    print("reparent err:", e)
unreal.EditorAssetLibrary.save_asset(abp.get_path_name())
cdo2 = unreal.get_default_object(abp.generated_class())
print("after: is ZP_MeleeHands:", isinstance(cdo2, unreal.ZP_MeleeHandsAnimInstance))
