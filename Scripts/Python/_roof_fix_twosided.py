import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LABEL = "Roof_Consolidated_Z3380"
BASE_MAT = "/Game/ResearchMegaPack/ResearchFacility/Materials/M_Concrete_5.M_Concrete_5"
MI_PATH = "/Game/TheSignal/Materials"
MI_NAME = "MI_Concrete_5_TwoSided"
MI_FULL = MI_PATH + "/" + MI_NAME + "." + MI_NAME

# --- create or load the two-sided material instance (additive, no overwrite of shared base) ---
if unreal.EditorAssetLibrary.does_asset_exist(MI_FULL):
    mi = unreal.load_asset(MI_FULL)
    print("Reusing existing MI:", MI_FULL)
else:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialInstanceConstantFactoryNew()
    mi = tools.create_asset(MI_NAME, MI_PATH, unreal.MaterialInstanceConstant, factory)
    print("Created MI:", MI_FULL)

mi.set_editor_property('parent', unreal.load_asset(BASE_MAT))

# set TwoSided base-property override
bpo = mi.get_editor_property('base_property_overrides')
bpo.set_editor_property('override_two_sided', True)
bpo.set_editor_property('two_sided', True)
mi.set_editor_property('base_property_overrides', bpo)

unreal.MaterialEditingLibrary.update_material_instance(mi)
unreal.EditorAssetLibrary.save_asset(MI_FULL)

# verify override took
bpo2 = mi.get_editor_property('base_property_overrides')
print("MI override_two_sided:", bpo2.get_editor_property('override_two_sided'),
      "| two_sided:", bpo2.get_editor_property('two_sided'))

# --- assign MI to both roof HISM components ---
actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        actor = a; break
if not actor:
    raise Exception("Roof actor missing")

for c in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
    c.set_material(0, mi)
    applied = c.get_material(0)
    print("  %s material[0] -> %s (instances=%d)" % (
        c.get_name(), applied.get_path_name() if applied else None, c.get_instance_count()))

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("SAVED level + MI.")
