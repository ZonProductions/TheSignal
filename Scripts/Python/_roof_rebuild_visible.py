import unreal, json

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
BACKUP = "C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_roof_backup_Z3380.json"
LABEL = "Roof_Consolidated_Z3380"
MI_FULL = "/Game/TheSignal/Materials/MI_Concrete_5_TwoSided.MI_Concrete_5_TwoSided"
BASE_MAT = "/Game/ResearchMegaPack/ResearchFacility/Materials/M_Concrete_5.M_Concrete_5"

if ues.get_editor_world() is None:
    raise Exception("ABORT: editor world None (PIE). Stop PIE first.")

# --- diagnose + destroy existing consolidated actor(s) ---
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        print("EXISTING '%s' class=%s temp_hidden=%s" % (
            a.get_name(), a.get_class().get_name(),
            a.is_temporarily_hidden_in_editor()))
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            print("   comp %s inst=%d visible=%s" % (c.get_name(), c.get_instance_count(),
                  c.get_editor_property('visible')))
        actor_sub.destroy_actor(a)
        print("   destroyed.")

# --- ensure two-sided material instance exists ---
if not unreal.EditorAssetLibrary.does_asset_exist(MI_FULL):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mi = tools.create_asset("MI_Concrete_5_TwoSided", "/Game/TheSignal/Materials",
                            unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property('parent', unreal.load_asset(BASE_MAT))
    bpo = mi.get_editor_property('base_property_overrides')
    bpo.set_editor_property('override_two_sided', True)
    bpo.set_editor_property('two_sided', True)
    mi.set_editor_property('base_property_overrides', bpo)
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_asset(MI_FULL)
mi = unreal.load_asset(MI_FULL)

# --- load transforms ---
with open(BACKUP) as f:
    tiles = json.load(f)["tiles"]
xforms = []
for t in tiles:
    xforms.append(unreal.Transform(
        unreal.Vector(*t["loc"]),
        unreal.Rotator(t["rot"][0], t["rot"][1], t["rot"][2]),
        unreal.Vector(*t["scale"])))
print("Transforms loaded:", len(xforms))

# --- spawn ONE actor, ONE plain ISM (renders all instances, no tree needed) ---
actor = actor_sub.spawn_actor_from_class(unreal.Actor, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
actor.set_actor_label(LABEL)
handles = sds.k2_gather_subobject_data_for_instance(actor)
p = unreal.AddNewSubobjectParams()
p.parent_handle = handles[0]
p.new_class = unreal.InstancedStaticMeshComponent
p.blueprint_context = None
h, fail = sds.add_new_subobject(p)
if not fail.is_empty():
    raise Exception("add_new_subobject failed: " + str(fail))
sds.rename_subobject(h, unreal.Text.cast("RoofISM"))
ism = unreal.InstancedStaticMeshComponent.cast(
    unreal.SubobjectDataBlueprintFunctionLibrary.get_object(
        unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)))
ism.set_static_mesh(unreal.load_asset(ROOF_MESH))
ism.set_mobility(unreal.ComponentMobility.STATIC)
ism.set_material(0, mi)                     # two-sided -> cannot be culled either side
ism.add_instances(xforms, False, True)      # world space
ism.set_editor_property('visible', True)

# not hidden
actor.set_editor_property('hidden', False)
actor.set_is_temporarily_hidden_in_editor(False)

print("RoofISM instances:", ism.get_instance_count(), "| material[0]:", ism.get_material(0).get_path_name())
print("Actor path:", actor.get_path_name())

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("SAVED.")
