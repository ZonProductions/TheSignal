import unreal
abp = unreal.load_asset("/Game/Marcus/ABP_MarcusMeleeHands")
gc = abp.generated_class()
print("current parent:", abp.get_editor_property("ParentClass") if False else gc.get_super_class().get_name())
# reparent to the C++ class
try:
    unreal.BlueprintEditorLibrary.reparent_blueprint(abp, unreal.ZP_MeleeHandsAnimInstance)
    print("reparented to:", abp.generated_class().get_super_class().get_name())
except Exception as e:
    print("reparent err:", e)
unreal.EditorAssetLibrary.save_asset(abp.get_path_name())
print("saved; parent now:", abp.generated_class().get_super_class().get_name())
