import unreal
bp = unreal.load_asset("/Game/HorrorLight/BP_HorrorLight")
mi = unreal.load_asset("/Game/HorrorLight/Materials/MI_FlickeringLight")
subsys = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsys.k2_gather_subobject_data_for_blueprint(bp)
print("handles:", len(handles))
done = False
for h in handles:
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if obj is None:
        continue
    name = obj.get_name()
    if "Cylinder" in name and isinstance(obj, unreal.StaticMeshComponent):
        print("found template:", name, type(obj).__name__)
        obj.set_material(0, mi)
        obj.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        try:
            mats = obj.get_editor_property("override_materials")
            print("  override_materials now:", [m.get_name() if m else None for m in mats])
        except Exception as e:
            print("  read back err:", e)
        done = True
print("modified:", done)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_asset("/Game/HorrorLight/BP_HorrorLight")
print("compiled + saved")
