import unreal
EAL = unreal.EditorAssetLibrary
SRC = "/Game/InventorySystemPro/ExampleContent/Common/Effects/Particles/NS_FogDust"
DST = "/Game/HorrorLight/NS_DustMote"

print("source exists:", EAL.does_asset_exist(SRC))
if EAL.does_asset_exist(DST):
    EAL.delete_asset(DST)
EAL.duplicate_asset(SRC, DST)
EAL.save_asset(DST)
print("NS_DustMote now = duplicate of NS_FogDust:", EAL.does_asset_exist(DST))

# Reassign to the NiagaraComponent template (path is same but object identity changed)
bp = unreal.load_asset("/Game/HorrorLight/BP_HorrorLight")
nsys = unreal.load_asset(DST)
subsys = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for h in subsys.k2_gather_subobject_data_for_blueprint(bp):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if obj and isinstance(obj, unreal.NiagaraComponent):
        obj.set_asset(nsys)
        obj.set_editor_property("auto_activate", True)
        a = obj.get_editor_property("asset")
        print("component asset:", a.get_name() if a else None)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
EAL.save_asset("/Game/HorrorLight/BP_HorrorLight")
print("BP recompiled + saved")
