import unreal
EAL = unreal.EditorAssetLibrary
SRC = "/Game/LEVELS/ModularSciFi/FX/Dust/FXS_Dust"
DST = "/Game/HorrorLight/NS_DustMote"

print("source exists:", EAL.does_asset_exist(SRC))
if EAL.does_asset_exist(DST):
    EAL.delete_asset(DST)
ok = EAL.duplicate_asset(SRC, DST)
print("duplicated NS_DustMote:", ok is not None)
EAL.save_asset(DST)

# Assign to the NiagaraComponent SCS template on BP_HorrorLight
bp = unreal.load_asset("/Game/HorrorLight/BP_HorrorLight")
nsys = unreal.load_asset(DST)
subsys = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsys.k2_gather_subobject_data_for_blueprint(bp)
assigned = False
for h in handles:
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if obj and isinstance(obj, unreal.NiagaraComponent):
        obj.set_asset(nsys)
        obj.set_editor_property("auto_activate", True)
        # lift the dust volume a bit so motes hang in the light cone
        obj.set_relative_location(unreal.Vector(0, 0, -150), False, False)
        cur = obj.get_editor_property("asset")
        print("niagara template asset now:", cur.get_name() if cur else None)
        assigned = True
print("assigned:", assigned)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
EAL.save_asset("/Game/HorrorLight/BP_HorrorLight")
print("compiled + saved BP")
