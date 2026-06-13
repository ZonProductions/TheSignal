import unreal
bp = unreal.load_asset("/Game/HorrorLight/BP_HorrorLight")
subsys = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsys.k2_gather_subobject_data_for_blueprint(bp)
print("=== BP_HorrorLight components ===")
seen=set()
for h in handles:
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if not obj: continue
    nm = obj.get_name()
    if nm in seen: continue
    seen.add(nm)
    line = "  {} :: {}".format(nm, type(obj).__name__)
    if isinstance(obj, unreal.SpotLightComponent):
        line += " | units={} intensity={} IES={} LF={}".format(
            obj.get_editor_property("intensity_units"),
            round(obj.get_editor_property("intensity"),2),
            obj.get_editor_property("ies_texture").get_name() if obj.get_editor_property("ies_texture") else None,
            obj.get_editor_property("light_function_material").get_name() if obj.get_editor_property("light_function_material") else None)
    if isinstance(obj, unreal.StaticMeshComponent) and "Cylinder" in nm:
        mats=[m.get_name() if m else None for m in obj.get_editor_property("override_materials")]
        line += " | mesh={} mats={}".format(obj.get_editor_property("static_mesh").get_name() if obj.get_editor_property("static_mesh") else None, mats)
    if isinstance(obj, unreal.NiagaraComponent):
        a=obj.get_editor_property("asset")
        line += " | asset={}".format(a.get_name() if a else None)
    print(line)
print("=== Vars ===")
